#!/usr/bin/env python2.5

#
# Copyright (c) 2009 Dr. D Studios. (Please refer to license for details)
# SVN_META_HEADURL = "$HeadURL: $"
# SVN_META_ID = "$Id: bach_server.py 9408 2010-03-03 22:35:49Z brobison $"
#

import sys
import os
from circuits.core import debugger
from sevan.server import SimpleServer
from sevan import server, dispatcher, transformer
from bach.controllers import artref
from bach import config

def main( argv = None ):
    if argv is None:
        argv = sys.argv[ 1: ]

    # pass arguments here, pretty please
    host = 'localhost'
    port = 8001
    views_root = os.path.join(os.path.dirname(__file__), 'views')

    # construct server
    server = SimpleServer("bach", static_path=views_root, host=host, port=port, database_session_maker=config.Session)

    # you must load models here
    from bach.models import asset

    # and the controller
    artrefController = artref.Artref()
    server += artrefController

    server.dispatcher.connect( 'index',
                      '',
                      controller=artrefController,
                      action='index' )

    server.dispatcher.connect( 'thumb',
                      '/thumb/*(path)',
                      controller=artrefController,
                      action='thumb' )

    server.dispatcher.connect( 'image',
                      'image',
                      controller=artrefController,
                      action='image' )

    server.dispatcher.connect( 'collections',
                      'collections',
                      controller=artrefController,
                      action='collections' )

    server.dispatcher.connect( 'keywords',
                      'keywords',
                      controller=artrefController,
                      action='keywords' )

    server.dispatcher.connect( 'collection',
                      'collection/{partId}',
                      controller=artrefController,
                      action='collection',
                      partId=None )

    server.dispatcher.connect( 'keyword',
                      'keyword/{partId}',
                      controller=artrefController,
                      action='keyword',
                      partId=None )

    server.run()

if __name__ == "__main__":
    sys.exit( main() )
