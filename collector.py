#! /usr/bin/env python3
# Server Collector: handles requests sent by the Workers and the Client.

# Possible requests:
#    MW: from MasterWorker, stores the given couple (sum, filename) in the couple list.
#    C1: from Client, sends the entire list of couples 
#    C2: from Client, sends the list of couples with given numbers

import sys, struct, socket, threading

HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 59643  # Port to listen on (non-privileged ports are > 1023)

c_list = [] # global list of couples

class ClientThread(threading.Thread):
    def __init__(self,conn,addr):
        threading.Thread.__init__(self)
        self.conn = conn
        self.addr = addr
    def run(self):
      handle_conn(self.conn,self.addr)
      
def main(host=HOST, port=PORT):
  # create socket
  with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    # bind port and address then wait
    s.bind((HOST,PORT))
    s.listen()
    while True:
      print("Waiting...")
      # start threads after accepting connection
      conn, addr=s.accept()
      t = ClientThread(conn,addr)
      t.start()
    s.shutdown(socket.SHUT_RDWR)

def handle_conn(conn,addr):
  # Before read string length
  temp = recv_all(conn,4)
  req_len = struct.unpack("!i", temp)[0]
  # then read the request
  temp = recv_all(conn,req_len)
  req = temp.decode('utf-8')
  
  # check request
  if req.__eq__("MW"):
    # Request: MW
    # this request needs the sum and the filename
    # read the sum one half at a time
    temp = recv_all(conn,8)
    sum = struct.unpack("!q", temp)[0]
    # then read the filename
    temp = recv_all(conn,4)
    f_len = struct.unpack("!i", temp)[0]
    temp = recv_all(conn,f_len)
    fn = temp.decode('utf-8')
    mw_req(conn,addr,sum,fn)
    
  elif req.__eq__("C1"):
    # Request: C1
    # this request doesn't need the sum nor the filename
    cl_req1(conn,addr)
  
  elif req.__eq__("C2"):
    # Request: C2
    # this request only needs the sum
    temp = recv_all(conn,8)
    sum = struct.unpack("!q", temp)[0]
    cl_req2(conn,addr,sum)
    
def mw_req(conn,addr,new_sum,new_fn): 
  # Request: MW 
  # store a given couple (sum, filename) in the couple list c_list
  for sum,fn in c_list:
    if(new_fn == fn):
      c_list.remove((sum, fn))
  c_list.append((new_sum,new_fn))
  c_list.sort()
  
def cl_req1(conn,addr):  
  # Request: C2
  # send all the couples (sum, filename)
  print("Req. C1: Sending...")
  # first sends the length of the list, then every couple (sum, filename)
  conn.sendall(struct.pack("!i", len(c_list)))
  for sum,fn in c_list:
    conn.sendall(struct.pack("!i", len(str(sum))))
    conn.sendall(str(sum).encode())
    conn.sendall(struct.pack("!i", len(fn)))
    conn.sendall(fn.encode())
  
def cl_req2(conn,addr,new_sum): 
  # Request: MW 
  # send every couple (sum, filename) with a given sum 
  # send 0 if there are no couples with that given sum
  print("Req. C2: Sending...")
  new_l=[] 
  for sum,fn in c_list:
    if new_sum == sum:
      new_l.append((sum,fn))
  if len(new_l)>0:
    # new_l not empty
    new_l.sort()
    # send new_l length first then send couples with the sum given
    conn.sendall(struct.pack("!i", len(new_l)))
    for sum,fn in new_l:
      conn.sendall(struct.pack("!i", len(str(sum))))
      conn.sendall(str(sum).encode())
      conn.sendall(struct.pack("!i", len(fn)))
      conn.sendall(fn.encode())
  else:
    # new_l is empty, send 0
    conn.sendall(struct.pack("!i", len(new_l)))
  
def recv_all(conn,n):
  chunks = b''
  bytes_recd=0
  while bytes_recd < n:
    chunk=conn.recv(min(n-bytes_recd, 1024))
    if len(chunk)==0:
      raise RuntimeError("Socket connection broken.")
    chunks+=chunk
    bytes_recd+= len(chunk)
  return chunks

if len(sys.argv)==1:
  main()
else:
  print("Uso:\n\t %s [host] [port]" % sys.argv[0])