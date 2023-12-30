sudo ./killdocker-33.sh
sudo docker build -f dockerfile-33 -t opcuaserver33:v1.0 --tag 'opcuaserver33' .
sudo docker run -it -d --name opcuaserver33 opcuaserver33 bash
sudo docker exec -it opcuaserver33 /bin/bash
