SERVER=cert
#socat -v openssl-listen:443,reuseaddr,cert=$SERVER/server.pem,cafile=$SERVER/client.crt TCP:localhost:8080
sudo socat openssl-listen:443,fork,reuseaddr,cert=$SERVER/server.pem,cafile=$SERVER/client.crt,verify=0 TCP:localhost:8080 &

python benews-srv.py batch.txt
