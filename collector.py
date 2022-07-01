#! /usr/bin/env python3
#Server Collector che con l'uso di threads gestisce le richieste dei client

import sys, struct, socket, threading

# host e porta di default
HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 59640  # Port to listen on (non-privileged ports are > 1023)

c_list = []

class ClientThread(threading.Thread):
    def __init__(self,conn,addr):
        threading.Thread.__init__(self)
        self.conn = conn
        self.addr = addr
    def run(self):
      handle_conn(self.conn,self.addr)
      
# main
def main(host=HOST, port=PORT):
  # creo il server socket
  with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    # unisco indirizzo e porta e sto in attesa
    s.bind((HOST,PORT))
    s.listen()
    while True:
      print("Waiting...")
      # dopo che ricevo la connessione faccio partire i thread
      conn, addr=s.accept()
      # TODO distinguere le varie richieste da MW e Client
      t = ClientThread(conn,addr)
      t.start()
    s.shutdown(socket.SHUT_RDWR)

def handle_conn(conn,addr):
  # ogni richiesta prevede l'ordinamento crescente per somma
  # print(f"Request by {addr}")
  # Before read string length
  temp = recv_all(conn,4)
  req_len = struct.unpack("!i", temp)[0]
  # then read the request
  temp = recv_all(conn,req_len)
  req = temp.decode('utf-8')
  # then read the sum one half at a time
  temp = recv_all(conn,8)
  sum = struct.unpack("!q", temp)[0]
  # then read the filename
  temp = recv_all(conn,4)
  f_len = struct.unpack("!i", temp)[0]
  temp = recv_all(conn,f_len)
  fn = temp.decode('utf-8')
  # Req. 1 (MW)
  if req.__eq__("MW"):
    mw_req(conn,addr,sum,fn)
  # Req. 2 (Client)
  elif req.__eq__("C1"):
    cl_req1(conn,addr)
  # Req. 3 (Client)
  elif req.__eq__("C2"):
    cl_req2(conn,addr,sum,fn)
    
def mw_req(conn,addr,sum,fn): # DONE and TESTED
  # Req. 1 (MW): non risponde, memorizza una data coppia somma|nomefile
  # print("Req. by MW. Sending...")
  c_list.append((sum,fn))
  c_list.sort()
  print("List: ",c_list)
  
def cl_req1(conn,addr):
  # Req. 2 (Client): elencare tutte le coppie somma|nomefile
  print("Req. C1: Sending...")
  # send the global list back to the client
  # first sends the length of the list, then every couple sum|filename
  conn.sendall(struck.pack("!i", len(c_list)))
  for c in c_list:
    conn.sendall(struck.pack("!i", c[0]))
    conn.sendall(c[1].encode())
  
def cl_req2(conn,addr, sum, fn):
  # Req. 3 (Client): elencare le coppie somma|nomefile con una certa somma
  print("Req. C2: Sending...")
  new_l=[]
  for c in c_list:
    if c.sum == sum:
      new_l.append((sum,fn))
  # TODO: send the new list back to the client
  for nc in new_l:
    conn.sendall(struck.pack("!i", nc[0]))
    conn.sendall(nc[1].encode())

def recv_all(conn,n):
  chunks = b''  #b sta per byte
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