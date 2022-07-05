#! /usr/bin/env python3
#Server Collector che con l'uso di threads gestisce le richieste dei client

import sys, struct, socket, threading

# host e porta di default
HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 59643  # Port to listen on (non-privileged ports are > 1023)

c_list = []

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
    # Req. 1 (MW)
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
    # Req. 2 (C1)
    cl_req1(conn,addr)
  
  elif req.__eq__("C2"):
    # Req. 3 (C2)
    # this request only needs the sum
    temp = recv_all(conn,8)
    sum = struct.unpack("!q", temp)[0]
    cl_req2(conn,addr,sum)
    
def mw_req(conn,addr,new_sum,new_fn): # DONE and TESTED
  # Req. 1 (MW): store a given couple (sum, filename) in the couple list c_list
  for sum,fn in c_list:
    if(new_fn == fn):
      c_list.remove((sum, fn))
  c_list.append((new_sum,new_fn))
  c_list.sort()
  
def cl_req1(conn,addr): # DONE and TESTED 
  # Req. 2 (C1): send all the couples (sum, filename)
  print("Req. C1: Sending...")
  # first sends the length of the list, then every couple (sum, filename)
  conn.sendall(struct.pack("!i", len(c_list)))
  for sum,fn in c_list:
    conn.sendall(struct.pack("!i", len(str(sum))))
    conn.sendall(str(sum).encode())
    conn.sendall(struct.pack("!i", len(fn)))
    conn.sendall(fn.encode())
  
def cl_req2(conn,addr,new_sum): # DONE and TESTED
  # Req. 3 (C2): send every couple (sum, filename) with a given sum, send "Nessun file" if there are no couples with that given sum
  print("Req. C2: Sending...")
  new_l=[]
  for sum,fn in c_list:
    if new_sum == sum:
      new_l.append((sum,fn))
  if len(new_l)>0:
    new_l.sort()
  # send length first then send couples
    conn.sendall(struct.pack("!i", len(new_l)))
    for sum,fn in new_l:
      conn.sendall(struct.pack("!i", len(str(sum))))
      conn.sendall(str(sum).encode())
      conn.sendall(struct.pack("!i", len(fn)))
      conn.sendall(fn.encode())
  else:
    conn.sendall(struct.pack("!i", len(new_l)))
  
def recv_all(conn,n):
  chunks = b''
  bytes_recd=0
  while bytes_recd < n:
    chunk=conn.recv(min(n-bytes_recd, 1024))
    if len(chunk)==0:
      raise RuntimeError("Socket connection broken")
    chunks+=chunk
    bytes_recd+= len(chunk)
  return chunks

if len(sys.argv)==1:
  main()
else:
  print("Uso:\n\t %s [host] [port]" % sys.argv[0])