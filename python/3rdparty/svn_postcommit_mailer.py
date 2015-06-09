#!/usr/bin/env python
#
# mailer.py: send email describing a commit
#
# $HeadURL: http://svn.collab.net/repos/svn/branches/1.2.x/tools/hook-scripts/mailer/mailer.py $
# $LastChangedDate: 2005-04-21 14:12:41 -0400 (Thu, 21 Apr 2005) $
# $LastChangedBy: breser $
# $LastChangedRevision: 14358 $
#
# USAGE: mailer.py commit      REPOS REVISION [CONFIG-FILE]
#        mailer.py propchange  REPOS REVISION AUTHOR PROPNAME [CONFIG-FILE]
#        mailer.py propchange2 REPOS REVISION AUTHOR PROPNAME ACTION \
#                              [CONFIG-FILE]
#        mailer.py lock        REPOS AUTHOR [CONFIG-FILE]
#        mailer.py unlock      REPOS AUTHOR [CONFIG-FILE]
#
#   Using CONFIG-FILE, deliver an email describing the changes between
#   REV and REV-1 for the repository REPOS.
#
#   ACTION was added as a fifth argument to the post-revprop-change hook
#   in Subversion 1.2.0.  Its value is one of 'A', 'M' or 'D' to indicate
#   if the property was added, modified or deleted, respectively.
#
#   This version of mailer.py requires the python bindings from
#   subversion 1.2.0 or later.
#

import os
import sys
import string
import ConfigParser
import time
import popen2
import cStringIO
import smtplib
import re
import tempfile
import types

import svn.fs
import svn.delta
import svn.repos
import svn.core

SEPARATOR = '=' * 78


def main(pool, cmd, config_fname, repos_dir, cmd_args):
  ### TODO:  Sanity check the incoming args
  
  if cmd == 'commit':
    revision = int(cmd_args[0])
    repos = Repository(repos_dir, revision, pool)
    cfg = Config(config_fname, repos, { 'author' : repos.author })
    messenger = Commit(pool, cfg, repos)
  elif cmd == 'propchange' or cmd == 'propchange2':
    revision = int(cmd_args[0])
    author = cmd_args[1]
    propname = cmd_args[2]
    action = (cmd == 'propchange2' and cmd_args[3] or 'A')
    repos = Repository(repos_dir, revision, pool)
    # Override the repos revision author with the author of the propchange
    repos.author = author
    cfg = Config(config_fname, repos, { 'author' : author })
    messenger = PropChange(pool, cfg, repos, author, propname, action)
  elif cmd == 'lock' or cmd == 'unlock':
    author = cmd_args[0]
    repos = Repository(repos_dir, 0, pool) ### any old revision will do
    cfg = Config(config_fname, repos, { 'author' : author })
    messenger = Lock(pool, cfg, repos, author, cmd == 'lock')
  else:
    raise UnknownSubcommand(cmd)

  messenger.generate()


# Minimal, incomplete, versions of popen2.Popen[34] for those platforms
# for which popen2 does not provide them.
try:
  Popen3 = popen2.Popen3
  Popen4 = popen2.Popen4
except AttributeError:
  class Popen3:
    def __init__(self, cmd, capturestderr = False):
      if type(cmd) != types.StringType:
        cmd = svn.core.argv_to_command_string(cmd)
      if capturestderr:
        self.fromchild, self.tochild, self.childerr \
            = popen2.popen3(cmd, mode='b')
      else:
        self.fromchild, self.tochild = popen2.popen2(cmd, mode='b')
        self.childerr = None

    def wait(self):
      rv = self.fromchild.close()
      rv = self.tochild.close() or rv
      if self.childerr is not None:
        rv = self.childerr.close() or rv
      return rv

  class Popen4:
    def __init__(self, cmd):
      if type(cmd) != types.StringType:
        cmd = svn.core.argv_to_command_string(cmd)
      self.fromchild, self.tochild = popen2.popen4(cmd, mode='b')

    def wait(self):
      rv = self.fromchild.close()
      rv = self.tochild.close() or rv
      return rv


class OutputBase:
  "Abstract base class to formalize the inteface of output methods"

  def __init__(self, cfg, repos, prefix_param):
    self.cfg = cfg
    self.repos = repos
    self.prefix_param = prefix_param
    self._CHUNKSIZE = 128 * 1024

    # This is a public member variable. This must be assigned a suitable
    # piece of descriptive text before make_subject() is called.
    self.subject = ""

  def make_subject(self, group, params):
    prefix = self.cfg.get(self.prefix_param, group, params)
    if prefix:
      subject = prefix + ' ' + self.subject
    else:
      subject = self.subject

    try:
      truncate_subject = int(
          self.cfg.get('truncate-subject', group, params))
    except ValueError:
      truncate_subject = 0

    if truncate_subject and len(subject) > truncate_subject:
      subject = subject[:(truncate_subject - 3)] + "..."
    return subject

  def start(self, group, params):
    """Override this method.
    Begin writing an output representation. GROUP is the name of the
    configuration file group which is causing this output to be produced.
    PARAMS is a dictionary of any named subexpressions of regular expressions
    defined in the configuration file, plus the key 'author' contains the
    author of the action being reported."""
    raise NotImplementedError

  def finish(self):
    """Override this method.
    Flush any cached information and finish writing the output
    representation."""
    raise NotImplementedError

  def write(self, output):
    """Override this method.
    Append the literal text string OUTPUT to the output representation."""
    raise NotImplementedError

  def run(self, cmd):
    """Override this method, if the default implementation is not sufficient.
    Execute CMD, writing the stdout produced to the output representation."""
    # By default we choose to incorporate child stderr into the output
    pipe_ob = Popen4(cmd)

    buf = pipe_ob.fromchild.read(self._CHUNKSIZE)
    while buf:
      self.write(buf)
      buf = pipe_ob.fromchild.read(self._CHUNKSIZE)

    # wait on the child so we don't end up with a billion zombies
    pipe_ob.wait()


class MailedOutput(OutputBase):
  def __init__(self, cfg, repos, prefix_param):
    OutputBase.__init__(self, cfg, repos, prefix_param)

  def start(self, group, params):
    # whitespace-separated list of addresses; split into a clean list:
    self.to_addrs = \
        filter(None, string.split(self.cfg.get('to_addr', group, params)))
    #self.from_addr = self.cfg.get('from_addr', group, params) \
    #                 or self.repos.author or 'no_author'
    self.from_addr = self.repos.author + '@blur.com' or 'no_author@blur.com'
    self.reply_to = self.cfg.get('reply_to', group, params)

  def mail_headers(self, group, params):
    subject = self.make_subject(group, params)
    hdrs = 'From: %s\n'    \
           'To: %s\n'      \
           'Subject: %s\n' \
           'MIME-Version: 1.0\n' \
           'Content-Type: text/plain; charset=UTF-8\n' \
           % (self.from_addr, string.join(self.to_addrs, ', '), subject)
    if self.reply_to:
      hdrs = '%sReply-To: %s\n' % (hdrs, self.reply_to)
    return hdrs + '\n'


class SMTPOutput(MailedOutput):
  "Deliver a mail message to an MTA using SMTP."

  def start(self, group, params):
    MailedOutput.start(self, group, params)

    self.buffer = cStringIO.StringIO()
    self.write = self.buffer.write

    self.write(self.mail_headers(group, params))

  def finish(self):
    server = smtplib.SMTP(self.cfg.general.smtp_hostname)
    if self.cfg.is_set('general.smtp_username'):
      server.login(self.cfg.general.smtp_username,
                   self.cfg.general.smtp_password)
    server.sendmail(self.from_addr, self.to_addrs, self.buffer.getvalue())
    server.quit()


class StandardOutput(OutputBase):
  "Print the commit message to stdout."

  def __init__(self, cfg, repos, prefix_param):
    OutputBase.__init__(self, cfg, repos, prefix_param)
    self.write = sys.stdout.write

  def start(self, group, params):
    self.write("Group: " + (group or "defaults") + "\n")
    self.write("Subject: " + self.make_subject(group, params) + "\n\n")

  def finish(self):
    pass


class PipeOutput(MailedOutput):
  "Deliver a mail message to an MDA via a pipe."

  def __init__(self, cfg, repos, prefix_param):
    MailedOutput.__init__(self, cfg, repos, prefix_param)

    # figure out the command for delivery
    self.cmd = string.split(cfg.general.mail_command)

  def start(self, group, params):
    MailedOutput.start(self, group, params)

    ### gotta fix this. this is pretty specific to sendmail and qmail's
    ### mailwrapper program. should be able to use option param substitution
    cmd = self.cmd + [ '-f', self.from_addr ] + self.to_addrs

    # construct the pipe for talking to the mailer
    self.pipe = Popen3(cmd)
    self.write = self.pipe.tochild.write

    # we don't need the read-from-mailer descriptor, so close it
    self.pipe.fromchild.close()

    # start writing out the mail message
    self.write(self.mail_headers(group, params))

  def finish(self):
    # signal that we're done sending content
    self.pipe.tochild.close()

    # wait to avoid zombies
    self.pipe.wait()


class Messenger:
  def __init__(self, pool, cfg, repos, prefix_param):
    self.pool = pool
    self.cfg = cfg
    self.repos = repos

    if cfg.is_set('general.mail_command'):
      cls = PipeOutput
    elif cfg.is_set('general.smtp_hostname'):
      cls = SMTPOutput
    else:
      cls = StandardOutput

    self.output = cls(cfg, repos, prefix_param)


class Commit(Messenger):
  def __init__(self, pool, cfg, repos):
    Messenger.__init__(self, pool, cfg, repos, 'commit_subject_prefix')

    # get all the changes and sort by path
    editor = svn.repos.ChangeCollector(repos.fs_ptr, repos.root_this, self.pool)
    e_ptr, e_baton = svn.delta.make_editor(editor, self.pool)
    svn.repos.replay(repos.root_this, e_ptr, e_baton, self.pool)

    self.changelist = editor.get_changes().items()
    self.changelist.sort()

    # collect the set of groups and the unique sets of params for the options
    self.groups = { }
    for path, change in self.changelist:
      for (group, params) in self.cfg.which_groups(path):
        # turn the params into a hashable object and stash it away
        param_list = params.items()
        param_list.sort()
        # collect the set of paths belonging to this group
        if self.groups.has_key( (group, tuple(param_list)) ):
          old_param, paths = self.groups[group, tuple(param_list)]
        else:
          paths = { }
        paths[path] = None
        self.groups[group, tuple(param_list)] = (params, paths)

    # figure out the changed directories
    dirs = { }
    for path, change in self.changelist:
      if change.item_kind == svn.core.svn_node_dir:
        dirs[path] = None
      else:
        idx = string.rfind(path, '/')
        if idx == -1:
          dirs[''] = None
        else:
          dirs[path[:idx]] = None

    dirlist = dirs.keys()

    commondir, dirlist = get_commondir(dirlist)

    # compose the basic subject line. later, we can prefix it.
    dirlist.sort()
    dirlist = string.join(dirlist)
    if commondir:
      self.output.subject = 'r%d - in %s: %s' % (repos.rev, commondir, dirlist)
    else:
      self.output.subject = 'r%d - %s' % (repos.rev, dirlist)

  def generate(self):
    "Generate email for the various groups and option-params."

    ### the groups need to be further compressed. if the headers and
    ### body are the same across groups, then we can have multiple To:
    ### addresses. SMTPOutput holds the entire message body in memory,
    ### so if the body doesn't change, then it can be sent N times
    ### rather than rebuilding it each time.

    subpool = svn.core.svn_pool_create(self.pool)

    # build a renderer, tied to our output stream
    renderer = TextCommitRenderer(self.output)

    for (group, param_tuple), (params, paths) in self.groups.items():
      self.output.start(group, params)

      # generate the content for this group and set of params
      generate_content(renderer, self.cfg, self.repos, self.changelist,
                       group, params, paths, subpool)

      self.output.finish()
      svn.core.svn_pool_clear(subpool)

    svn.core.svn_pool_destroy(subpool)


try:
  from tempfile import NamedTemporaryFile
except ImportError:
  # NamedTemporaryFile was added in Python 2.3, so we need to emulate it
  # for older Pythons.
  class NamedTemporaryFile:
    def __init__(self):
      self.name = tempfile.mktemp()
      self.file = open(self.name, 'w+b')
    def __del__(self):
      os.remove(self.name)
    def write(self, data):
      self.file.write(data)
    def flush(self):
      self.file.flush()


class PropChange(Messenger):
  def __init__(self, pool, cfg, repos, author, propname, action):
    Messenger.__init__(self, pool, cfg, repos, 'propchange_subject_prefix')
    self.author = author
    self.propname = propname
    self.action = action

    # collect the set of groups and the unique sets of params for the options
    self.groups = { }
    for (group, params) in self.cfg.which_groups(''):
      # turn the params into a hashable object and stash it away
      param_list = params.items()
      param_list.sort()
      self.groups[group, tuple(param_list)] = params

    self.output.subject = 'r%d - %s' % (repos.rev, propname)

  def generate(self):
    actions = { 'A': 'added', 'M': 'modified', 'D': 'deleted' }
    for (group, param_tuple), params in self.groups.items():
      self.output.start(group, params)
      self.output.write('Author: %s\n'
                        'Revision: %s\n'
                        'Property Name: %s\n'
                        'Action: %s\n'
                        '\n'
                        % (self.author, self.repos.rev, self.propname,
                           actions.get(self.action, 'Unknown (\'%s\')' \
                                       % self.action)))
      if self.action == 'A' or not actions.has_key(self.action):
        self.output.write('Property value:\n')
        propvalue = self.repos.get_rev_prop(self.propname)
        self.output.write(propvalue)
      elif self.action == 'M':
        self.output.write('Property diff:\n')
        tempfile1 = NamedTemporaryFile()
        tempfile1.write(sys.stdin.read())
        tempfile1.flush()
        tempfile2 = NamedTemporaryFile()
        tempfile2.write(self.repos.get_rev_prop(self.propname))
        tempfile2.flush()
        self.output.run(self.cfg.get_diff_cmd(group, {
          'label_from' : 'old property value',
          'label_to' : 'new property value',
          'from' : tempfile1.name,
          'to' : tempfile2.name,
          }))
      self.output.finish()


def get_commondir(dirlist):
  """Figure out the common portion/parent (commondir) of all the paths
  in DIRLIST and return a tuple consisting of commondir, dirlist.  If
  a commondir is found, the dirlist returned is rooted in that
  commondir.  If no commondir is found, dirlist is returned unchanged,
  and commondir is the empty string."""
  if len(dirlist) == 1 or '/' in dirlist:
    commondir = ''
    newdirs = dirlist
  else:
    common = string.split(dirlist.pop(), '/')
    for d in dirlist:
      parts = string.split(d, '/')
      for i in range(len(common)):
        if i == len(parts) or common[i] != parts[i]:
          del common[i:]
          break
    commondir = string.join(common, '/')
    if commondir:
      # strip the common portion from each directory
      l = len(commondir) + 1
      newdirs = [ ]
      for d in dirlist:
        if d == commondir:
          newdirs.append('.')
        else:
          newdirs.append(d[l:])
    else:
      # nothing in common, so reset the list of directories
      newdirs = dirlist

  return commondir, newdirs


class Lock(Messenger):
  def __init__(self, pool, cfg, repos, author, do_lock):
    self.author = author
    self.do_lock = do_lock

    Messenger.__init__(self, pool, cfg, repos,
                       (do_lock and 'lock_subject_prefix'
                        or 'unlock_subject_prefix'))

    # read all the locked paths from STDIN and strip off the trailing newlines
    self.dirlist = map(lambda x: x.rstrip(), sys.stdin.readlines())

    # collect the set of groups and the unique sets of params for the options
    self.groups = { }
    for path in self.dirlist:
      for (group, params) in self.cfg.which_groups(path):
        # turn the params into a hashable object and stash it away
        param_list = params.items()
        param_list.sort()
        # collect the set of paths belonging to this group
        if self.groups.has_key( (group, tuple(param_list)) ):
          old_param, paths = self.groups[group, tuple(param_list)]
        else:
          paths = { }
        paths[path] = None
        self.groups[group, tuple(param_list)] = (params, paths)

    commondir, dirlist = get_commondir(self.dirlist)

    # compose the basic subject line. later, we can prefix it.
    dirlist.sort()
    dirlist = string.join(dirlist)
    if commondir:
      self.output.subject = '%s: %s' % (commondir, dirlist)
    else:
      self.output.subject = '%s' % (dirlist)

    # The lock comment is the same for all paths, so we can just pull
    # the comment for the first path in the dirlist and cache it.
    self.lock = svn.fs.svn_fs_get_lock(self.repos.fs_ptr,
                                       self.dirlist[0], self.pool)

  def generate(self):
    for (group, param_tuple), (params, paths) in self.groups.items():
      self.output.start(group, params)

      self.output.write('Author: %s\n'
                        '%s paths:\n' %
                        (self.author, self.do_lock and 'Locked' or 'Unlocked'))

      self.dirlist.sort()
      for dir in self.dirlist:
        self.output.write('   %s\n\n' % dir)

      if self.do_lock:
        self.output.write('Comment:\n%s\n' % (self.lock.comment or ''))

      self.output.finish()


class DiffSelections:
  def __init__(self, cfg, group, params):
    self.add = False
    self.copy = False
    self.delete = False
    self.modify = False

    gen_diffs = cfg.get('generate_diffs', group, params)

    ### Do a little dance for deprecated options.  Note that even if you
    ### don't have an option anywhere in your configuration file, it
    ### still gets returned as non-None.
    if len(gen_diffs):
      list = string.split(gen_diffs, " ")
      for item in list:
        if item == 'add':
          self.add = True
        if item == 'copy':
          self.copy = True
        if item == 'delete':
          self.delete = True
        if item == 'modify':
          self.modify = True
    else:
      self.add = True
      self.copy = True
      self.delete = True
      self.modify = True
      ### These options are deprecated
      suppress = cfg.get('suppress_deletes', group, params)
      if suppress == 'yes':
        self.delete = False
      suppress = cfg.get('suppress_adds', group, params)
      if suppress == 'yes':
        self.add = False


def generate_content(renderer, cfg, repos, changelist, group, params, paths,
                     pool):

  svndate = repos.get_rev_prop(svn.core.SVN_PROP_REVISION_DATE)
  ### pick a different date format?
  date = time.ctime(svn.core.secs_from_timestr(svndate, pool))

  diffsels = DiffSelections(cfg, group, params)

  show_nonmatching_paths = cfg.get('show_nonmatching_paths', group, params) \
      or 'yes'

  # figure out the lists of changes outside the selected path-space
  other_added_data = other_removed_data = other_modified_data = [ ]
  if len(paths) != len(changelist) and show_nonmatching_paths != 'no':
    other_added_data = generate_list('A', changelist, paths, False)
    other_removed_data = generate_list('R', changelist, paths, False)
    other_modified_data = generate_list('M', changelist, paths, False)

  if len(paths) != len(changelist) and show_nonmatching_paths == 'yes':
    other_diffs = DiffGenerator(changelist, paths, False, cfg, repos, date,
                                group, params, diffsels, pool),
  else:
    other_diffs = [ ]

  data = _data(
    author=repos.author,
    date=date,
    rev=repos.rev,
    log=repos.get_rev_prop(svn.core.SVN_PROP_REVISION_LOG) or '',
    added_data=generate_list('A', changelist, paths, True),
    removed_data=generate_list('R', changelist, paths, True),
    modified_data=generate_list('M', changelist, paths, True),
    other_added_data=other_added_data,
    other_removed_data=other_removed_data,
    other_modified_data=other_modified_data,
    diffs=DiffGenerator(changelist, paths, True, cfg, repos, date, group,
                        params, diffsels, pool),
    other_diffs=other_diffs,
    )
  renderer.render(data)


def generate_list(changekind, changelist, paths, in_paths):
  if changekind == 'A':
    selection = lambda change: change.added
  elif changekind == 'R':
    selection = lambda change: change.path is None
  elif changekind == 'M':
    selection = lambda change: not change.added and change.path is not None

  items = [ ]
  for path, change in changelist:
    if selection(change) and paths.has_key(path) == in_paths:
      item = _data(
        path=path,
        is_dir=change.item_kind == svn.core.svn_node_dir,
        props_changed=change.prop_changes,
        text_changed=change.text_changed,
        copied=change.added and change.base_path,
        base_path=change.base_path,
        base_rev=change.base_rev,
        )
      items.append(item)

  return items


class DiffGenerator:
  "This is a generator-like object returning DiffContent objects."

  def __init__(self, changelist, paths, in_paths, cfg, repos, date, group,
               params, diffsels, pool):
    self.changelist = changelist
    self.paths = paths
    self.in_paths = in_paths
    self.cfg = cfg
    self.repos = repos
    self.date = date
    self.group = group
    self.params = params
    self.diffsels = diffsels
    self.pool = pool

    self.idx = 0

  def __nonzero__(self):
    # we always have some items
    return True

  def __getitem__(self, idx):
    while 1:
      if self.idx == len(self.changelist):
        raise IndexError

      path, change = self.changelist[self.idx]
      self.idx = self.idx + 1

      # just skip directories. they have no diffs.
      if change.item_kind == svn.core.svn_node_dir:
        continue

      # is this change in (or out of) the set of matched paths?
      if self.paths.has_key(path) != self.in_paths:
        continue

      # figure out if/how to generate a diff

      if not change.path:
        # it was deleted. should we show deletion diffs?
        if not self.diffsels.delete:
          continue

        kind = 'R'
        diff = svn.fs.FileDiff(self.repos.get_root(change.base_rev),
                               change.base_path, None, None, self.pool)

        label1 = '%s\t%s' % (change.base_path, self.date)
        label2 = '(empty file)'
        singular = True
      elif change.added:
        if change.base_path and (change.base_rev != -1):
          # this file was copied. any diff to show? should we?
          if not change.text_changed or not self.diffsels.copy:
            continue

          kind = 'C'
          diff = svn.fs.FileDiff(self.repos.get_root(change.base_rev),
                                 change.base_path,
                                 self.repos.root_this, change.path,
                                 self.pool)
          label1 = change.base_path + '\t(original)'
          label2 = '%s\t%s' % (change.path, self.date)
          singular = False
        else:
          # the file was added. should we show it?
          if not self.diffsels.add:
            continue

          kind = 'A'
          diff = svn.fs.FileDiff(None, None, self.repos.root_this,
                                 change.path, self.pool)
          label1 = '(empty file)'
          label2 = '%s\t%s' % (change.path, self.date)
          singular = True

      elif not change.text_changed:
        # the text didn't change, so nothing to show.
        continue
      else:
        # a simple modification. show the diff?
        if not self.diffsels.modify:
          continue

        kind = 'M'
        diff = svn.fs.FileDiff(self.repos.get_root(change.base_rev),
                               change.base_path,
                               self.repos.root_this, change.path,
                               self.pool)
        label1 = change.base_path + '\t(original)'
        label2 = '%s\t%s' % (change.path, self.date)
        singular = False

      binary = diff.either_binary()
      if binary:
        content = src_fname = dst_fname = None
      else:
        src_fname, dst_fname = diff.get_files()
        content = DiffContent(self.cfg.get_diff_cmd(self.group, {
          'label_from' : label1,
          'label_to' : label2,
          'from' : src_fname,
          'to' : dst_fname,
          }))

      # return a data item for this diff
      return _data(
        kind=kind,
        path=change.path,
        base_path=change.base_path,
        base_rev=change.base_rev,
        label_from=label1,
        label_to=label2,
        from_fname=src_fname,
        to_fname=dst_fname,
        binary=binary,
        singular=singular,
        content=content,
        diff=diff,
        )


class DiffContent:
  "This is a generator-like object returning annotated lines of a diff."

  def __init__(self, cmd):
    self.seen_change = False

    # By default we choose to incorporate child stderr into the output
    self.pipe = Popen4(cmd)

  def __nonzero__(self):
    # we always have some items
    return True

  def __getitem__(self, idx):
    if self.pipe is None:
      raise IndexError

    line = self.pipe.fromchild.readline()
    if not line:
      # wait on the child so we don't end up with a billion zombies
      self.pipe.wait()
      self.pipe = None
      raise IndexError

    # classify the type of line.
    first = line[:1]
    if first == '@':
      self.seen_change = True
      ltype = 'H'
    elif first == '-':
      if self.seen_change:
        ltype = 'D'
      else:
        ltype = 'F'
    elif first == '+':
      if self.seen_change:
        ltype = 'A'
      else:
        ltype = 'T'
    elif first == ' ':
      ltype = 'C'
    else:
      ltype = 'U'

    return _data(
      raw=line,
      text=line[1:-1],  # remove indicator and newline
      type=ltype,
      )


class TextCommitRenderer:
  "This class will render the commit mail in plain text."

  def __init__(self, output):
    self.output = output

  def render(self, data):
    "Render the commit defined by 'data'."

    w = self.output.write

    w('Author: %s\nDate: %s\nNew Revision: %s\n\n'
      % (data.author, data.date, data.rev))

    # print summary sections
    self._render_list('Added', data.added_data)
    self._render_list('Removed', data.removed_data)
    self._render_list('Modified', data.modified_data)

    if data.other_added_data or data.other_removed_data \
           or data.other_modified_data:
      if data.show_nonmatching_paths:
        w('\nChanges in other areas also in this revision:\n')
        self._render_list('Added', data.other_added_data)
        self._render_list('Removed', data.other_removed_data)
        self._render_list('Modified', data.other_modified_data)
      else:
        w('and changes in other areas\n')

    regexp1 = re.compile("(bug\s)(\d+)", re.IGNORECASE)
    regexp2 = re.compile('(issue\s)(\d+)', re.IGNORECASE)
    data.log = regexp1.sub("\\1\\2 (http://bugzilla.blur.com/show_bug.cgi?id=\\2)", data.log, 0);
    data.log = regexp2.sub('\\1\\2 (http://bugzilla.blur.com/show_bug.cgi?id=\\2)', data.log, 0)

    w('\nLog:\n%s\n' % data.log)

    self._render_diffs(data.diffs)
    if data.other_diffs:
      w('\nDiffs of changes in other areas also in this revision:\n')
      self._render_diffs(data.other_diffs)

  def _render_list(self, header, data_list):
    if not data_list:
      return

    w = self.output.write
    w(header + ':\n')
    for d in data_list:
      if d.is_dir:
        is_dir = '/'
      else:
        is_dir = ''
      if d.props_changed:
        if d.text_changed:
          props = '   (contents, props changed)'
        else:
          props = '   (props changed)'
      else:
        props = ''
      w('   %s%s%s\n' % (d.path, is_dir, props))
      if d.copied:
        if is_dir:
          text = ''
        elif d.text_changed:
          text = ', changed'
        else:
          text = ' unchanged'
        w('      - copied%s from r%d, %s%s\n'
          % (text, d.base_rev, d.base_path, is_dir))

  def _render_diffs(self, diffs):
    w = self.output.write

    for diff in diffs:
      if diff.kind == 'D':
        w('\nDeleted: %s\n' % diff.base_path)
      elif diff.kind == 'C':
        w('\nCopied: %s (from r%d, %s)\n'
          % (diff.path, diff.base_rev, diff.base_path))
      elif diff.kind == 'A':
        w('\nAdded: %s\n' % diff.path)
      else:
        # kind == 'M'
        w('\nModified: %s\n' % diff.path)

      w(SEPARATOR + '\n')

      if diff.binary:
        if diff.singular:
          w('Binary file. No diff available.\n')
        else:
          w('Binary files. No diff available.\n')
        continue

      for line in diff.content:
        w(line.raw)


class Repository:
  "Hold roots and other information about the repository."

  def __init__(self, repos_dir, rev, pool):
    self.repos_dir = repos_dir
    self.rev = rev
    self.pool = pool

    self.repos_ptr = svn.repos.open(repos_dir, pool)
    self.fs_ptr = svn.repos.fs(self.repos_ptr)

    self.roots = { }

    self.root_this = self.get_root(rev)

    self.author = self.get_rev_prop(svn.core.SVN_PROP_REVISION_AUTHOR)

  def get_rev_prop(self, propname):
    return svn.fs.revision_prop(self.fs_ptr, self.rev, propname, self.pool)

  def get_root(self, rev):
    try:
      return self.roots[rev]
    except KeyError:
      pass
    root = self.roots[rev] = svn.fs.revision_root(self.fs_ptr, rev, self.pool)
    return root


class Config:

  # The predefined configuration sections. These are omitted from the
  # set of groups.
  _predefined = ('general', 'defaults', 'maps')

  def __init__(self, fname, repos, global_params):
    cp = ConfigParser.ConfigParser()
    cp.read(fname)

    # record the (non-default) groups that we find
    self._groups = [ ]

    for section in cp.sections():
      if not hasattr(self, section):
        section_ob = _sub_section()
        setattr(self, section, section_ob)
        if section not in self._predefined:
          self._groups.append(section)
      else:
        section_ob = getattr(self, section)
      for option in cp.options(section):
        # get the raw value -- we use the same format for *our* interpolation
        value = cp.get(section, option, raw=1)
        setattr(section_ob, option, value)

    # be compatible with old format config files
    if hasattr(self.general, 'diff') and not hasattr(self.defaults, 'diff'):
      self.defaults.diff = self.general.diff
    if not hasattr(self, 'maps'):
      self.maps = _sub_section()

    # these params are always available, although they may be overridden
    self._global_params = global_params.copy()

    # prepare maps. this may remove sections from consideration as a group.
    self._prep_maps()

    # process all the group sections.
    self._prep_groups(repos)

  def is_set(self, option):
    """Return None if the option is not set; otherwise, its value is returned.

    The option is specified as a dotted symbol, such as 'general.mail_command'
    """
    ob = self
    for part in string.split(option, '.'):
      if not hasattr(ob, part):
        return None
      ob = getattr(ob, part)
    return ob

  def get(self, option, group, params):
    "Get a config value with appropriate substitutions and value mapping."

    # find the right value
    value = None
    if group:
      sub = getattr(self, group)
      value = getattr(sub, option, None)
    if value is None:
      value = getattr(self.defaults, option, '')
    
    # parameterize it
    if params is not None:
      value = value % params

    # apply any mapper
    mapper = getattr(self.maps, option, None)
    if mapper is not None:
      value = mapper(value)

    return value

  def get_diff_cmd(self, group, args):
    "Get a diff command as a list of argv elements."
    ### do some better splitting to enable quoting of spaces
    diff_cmd = string.split(self.get('diff', group, None))

    cmd = [ ]
    for part in diff_cmd:
      cmd.append(part % args)
    return cmd

  def _prep_maps(self):
    "Rewrite the [maps] options into callables that look up values."

    for optname, mapvalue in vars(self.maps).items():
      if mapvalue[:1] == '[':
        # a section is acting as a mapping
        sectname = mapvalue[1:-1]
        if not hasattr(self, sectname):
          raise UnknownMappingSection(sectname)
        # construct a lambda to look up the given value as an option name,
        # and return the option's value. if the option is not present,
        # then just return the value unchanged.
        setattr(self.maps, optname,
                lambda value,
                       sect=getattr(self, sectname): getattr(sect, value,
                                                             value))
        # remove the mapping section from consideration as a group
        self._groups.remove(sectname)

      # elif test for other mapper types. possible examples:
      #   dbm:filename.db
      #   file:two-column-file.txt
      #   ldap:some-query-spec
      # just craft a mapper function and insert it appropriately

      else:
        raise UnknownMappingSpec(mapvalue)

  def _prep_groups(self, repos):
    self._group_re = [ ]

    repos_dir = os.path.abspath(repos.repos_dir)

    # compute the default repository-based parameters. start with some
    # basic parameters, then bring in the regex-based params.
    default_params = self._global_params.copy()

    try:
      match = re.match(self.defaults.for_repos, repos_dir)
      if match:
        default_params.update(match.groupdict())
    except AttributeError:
      # there is no self.defaults.for_repos
      pass

    # select the groups that apply to this repository
    for group in self._groups:
      sub = getattr(self, group)
      params = default_params
      if hasattr(sub, 'for_repos'):
        match = re.match(sub.for_repos, repos_dir)
        if not match:
          continue
        params = self._global_params.copy()
        params.update(match.groupdict())

      # if a matching rule hasn't been given, then use the empty string
      # as it will match all paths
      for_paths = getattr(sub, 'for_paths', '')
      exclude_paths = getattr(sub, 'exclude_paths', None)
      if exclude_paths:
        exclude_paths_re = re.compile(exclude_paths)
      else:
        exclude_paths_re = None

      self._group_re.append((group, re.compile(for_paths),
                             exclude_paths_re, params))

    # after all the groups are done, add in the default group
    try:
      self._group_re.append((None,
                             re.compile(self.defaults.for_paths),
                             None,
                             default_params))
    except AttributeError:
      # there is no self.defaults.for_paths
      pass

  def which_groups(self, path):
    "Return the path's associated groups."
    groups = []
    for group, pattern, exclude_pattern, repos_params in self._group_re:
      match = pattern.match(path)
      if match:
        if exclude_pattern and exclude_pattern.match(path):
          continue
        params = repos_params.copy()
        params.update(match.groupdict())
        groups.append((group, params))
    if not groups:
      groups.append((None, self._global_params))
    return groups


class _sub_section:
  pass

class _data:
  "Helper class to define an attribute-based hunk o' data."
  def __init__(self, **kw):
    vars(self).update(kw)

class MissingConfig(Exception):
  pass
class UnknownMappingSection(Exception):
  pass
class UnknownMappingSpec(Exception):
  pass
class UnknownSubcommand(Exception):
  pass


# enable True/False in older vsns of Python
try:
  _unused = True
except NameError:
  True = 1
  False = 0


if __name__ == '__main__':
  def usage():
    sys.stderr.write(
"""USAGE: %s commit      REPOS REVISION [CONFIG-FILE]
       %s propchange  REPOS REVISION AUTHOR PROPNAME [CONFIG-FILE]
       %s propchange2 REPOS REVISION AUTHOR PROPNAME ACTION
       %s             [CONFIG-FILE]
       %s lock        REPOS AUTHOR [CONFIG-FILE]
       %s unlock      REPOS AUTHOR [CONFIG-FILE]

If no CONFIG-FILE is provided, the script will first search for a mailer.conf
file in REPOS/conf/.  Failing that, it will search the directory in which
the script itself resides.

ACTION was added as a fifth argument to the post-revprop-change hook
in Subversion 1.2.0.  Its value is one of 'A', 'M' or 'D' to indicate
if the property was added, modified or deleted, respectively.

""" % (sys.argv[0], sys.argv[0], sys.argv[0], ' ' * len(sys.argv[0]),
       sys.argv[0], sys.argv[0]))
    sys.exit(1)

  # Command list:  subcommand -> number of arguments expected (not including
  #                              the repository directory and config-file)
  cmd_list = {'commit'     : 1,
              'propchange' : 3,
              'propchange2': 4,
              'lock'       : 1,
              'unlock'     : 1,
              }

  config_fname = None
  argc = len(sys.argv)
  if argc < 3:
    usage()

  cmd = sys.argv[1]
  repos_dir = sys.argv[2]
  try:
    expected_args = cmd_list[cmd]
  except KeyError:
    usage()

  if argc < (expected_args + 3):
    usage()
  elif argc > expected_args + 4:
    usage()
  elif argc == (expected_args + 4):
    config_fname = sys.argv[expected_args + 3]

  # Settle on a config file location, and open it.
  if config_fname is None:
    # Default to REPOS-DIR/conf/mailer.conf.
    config_fname = os.path.join(repos_dir, 'conf', 'mailer.conf')
    if not os.path.exists(config_fname):
      # Okay.  Look for 'mailer.conf' as a sibling of this script.
      config_fname = os.path.join(os.path.dirname(sys.argv[0]), 'mailer.conf')
  if not os.path.exists(config_fname):
    raise MissingConfig(config_fname)
  config_fp = open(config_fname)

  svn.core.run_app(main, cmd, config_fname, repos_dir,
                   sys.argv[3:3+expected_args])

# ------------------------------------------------------------------------
# TODO
#
# * add configuration options
#   - each group defines delivery info:
#     o whether to set Reply-To and/or Mail-Followup-To
#       (btw: it is legal do set Reply-To since this is the originator of the
#        mail; i.e. different from MLMs that munge it)
#   - each group defines content construction:
#     o max size of diff before trimming
#     o max size of entire commit message before truncation
#   - per-repository configuration
#     o extra config living in repos
#     o how to construct a ViewCVS URL for the diff  [DONE (as patch)]
#     o optional, non-mail log file
#     o look up authors (username -> email; for the From: header) in a
#       file(s) or DBM
#   - if the subject line gets too long, then trim it. configurable?
# * get rid of global functions that should properly be class methods
