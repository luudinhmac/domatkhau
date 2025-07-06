#!bin/bash
for i in {1..1000000};
do     
  openssl rand -base64 24 | cut -c1-$((RANDOM % 32 + 8));
done > /etc/lists.txt

