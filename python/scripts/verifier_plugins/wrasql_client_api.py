#!/usr/bin/python2.5

import os.path
import sys
import asyncore, socket
import collections
import time

import wenv

class wrasql_client(asyncore.dispatcher):
	result = ""

	def __init__(self, host, sck_map):
		asyncore.dispatcher.__init__(self, map=sck_map)
		self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
		self.connect((host, wenv.wsql_master_port))
		self.buffer = ""
		self.outbox = collections.deque()
		self.inbox = collections.deque()
		self.sck_map = sck_map

	def handle_connect(self):
		pass

	def handle_close(self):
		self.close()

	def handle_read(self):
		self.setblocking(0)
		buffer = []
		try:
			data = self.recv(8192)
			while len(data) > 0:
				buffer.append(data)
				data = self.recv(8192)
		except:
			pass

		mesg = "".join(buffer)
		self.inbox.append(mesg)

	def writable(self):
		return len(self.outbox) > 0

	def handle_write(self):
		if not self.outbox:
			return

		self.setblocking(1)
		message = self.outbox.popleft()
		count = self.send(message)
		while count < len(message):
			count += self.send(message[count:])

	def query(self, query):
		self.outbox.append(query)

	def query_has_results(self):
		return len(self.inbox) > 0

	def query_result(self):
		if len(self.inbox) > 0:
			return self.inbox.popleft()
		
		return ""

	def poll_for_query_results(self):
		while not self.query_has_results() and self.readable():
			asyncore.loop(timeout=0.01, count=1, map=self.sck_map)

		result = ""
		while self.query_has_results() and self.readable():
			result += self.query_result()
			asyncore.loop(timeout=0.01, count=1, map=self.sck_map)

		try:
			exec("finalResult = %s" % result)
		except SyntaxError:
			finalResult = []
		return finalResult

	def queryAndClose(self, query):
		self.query(query)
		result = self.poll_for_query_results()
		self.close()
		return result
