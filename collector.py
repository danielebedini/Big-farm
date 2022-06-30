#! /usr/bin/env python3
#Server Collector che con l'uso di threads gestisce le richieste dei client

import sys, struct, socket, threading

# host e porta di default
HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 59675  # Port to listen on (non-privileged ports are > 1023)

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
      print("Request recived")
      # TODO distinguere le varie richieste da MW e Client
      t = ClientThread(conn,addr)
      t.start()
    s.shutdown(socket.SHUT_RDWR)

def handle_conn(conn,addr):
  with conn:
    print("Handling connection") 
    # ogni richiesta prevede l'ordinamento crescente per somma
    # print(f"Request by {addr}")
    # Before read string length
    temp = recv_all(conn,4)
    req_len = struct.unpack("!i", temp[:4])[0]
    print("req_len 1: ",req_len)
    temp = recv_all(conn,req_len)
    req = temp.decode('utf-8') # -> req can be: MW, C1, C2
    print("req: ",req)
    # Req. 1 (MW)
    if req.__eq__("MW"):
      mw_req(conn,addr)
    # Req. 2 (Client)
    elif req.__eq__("C1"):
      cl_req1(conn,addr)
    # Req. 3 (Client)
    elif req.__eq__("C2"):
      cl_req2(conn,addr)
    
def mw_req(conn,addr):
  # Req. 1 (MW): non risponde, memorizza una data coppia somma|nomefile
  print("Req. by MW. Sending...")
  
  
def cl_req1(conn,addr):
  # Req. 2 (Client): elencare tutte le coppie somma|nomefile
  print("Req. by C1. Sending...")
  
def cl_req2(conn,addr):
  # Req. 3 (Client): elencare le coppie somma|nomefile con una certa somma
  print("Req. by C2. Sending...")
  
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