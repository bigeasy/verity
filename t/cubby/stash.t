#!/bin/bash

echo "1..12"

function ok()
{
    local message=$1 status=$?;
    if [ $status -eq 0 ]; then
        status="ok"
    else
        status="not ok"
    fi
    if [ ! -z "$message" ]; then
        echo "$status $message" 
    else
        echo "$status"
    fi
}


bin/cubby > key &
pid=$!
sleep 1
key=$(cat key)

[ ! -z "$key" ];      ok "shutdown key is not zero"
[ ${#key} -eq 64 ];   ok "shutdown key is 64 chars long"

query="uri=http://foo.com/login"
query="$query&name=verity"
token=$(curl -s "http://localhost:8089/cubby/token?$query")

[ ! -z "$token" ];      ok "token is not zero"
[ ${#token} -eq 64 ];   ok "token is 64 chars long"

message=$(curl -d "alert('a');" -s "http://localhost:8089/cubby/data?token=$token")
[ "$message" == "Stashed." ];       ok "data stashed"

message=$(curl -d "alert('a');" -s "http://localhost:8089/cubby/data?token=${token}_")
[ "$message" == "Invalid token." ]; ok "token too long on stash"

message=$(curl -d "alert('a');" -s "http://localhost:8089/cubby/data?token=${token}A")
[ "$message" == "Invalid token." ]; ok "token not found on stash"

message=$(curl -s "http://localhost:8089/cubby/verity.js?http://foo.com/login")
[ "$message" == "alert('a');" ];    ok "get stashed data # ${message}"

query="uri=http://foo.com/login"
query="$query&name=other"
token=$(curl -s "http://localhost:8089/cubby/token?$query")
[ ${#token} -eq 64 ];   ok "other token"

message=$(curl -d "alert('b');" -s "http://localhost:8089/cubby/data?token=$token")
[ "$message" == "Stashed." ];       ok "other data stashed"

message=$(curl -s "http://localhost:8089/cubby/other.js?http://foo.com/login")
[ "$message" == "alert('b');" ];    ok "get other data # ${message}"

query="uri=http://foo.com/login"
query="$query&name=verity"
token=$(curl -s "http://localhost:8089/cubby/token?$query")
[ ${#token} -eq 64 ];   ok "reset token"

message=$(curl -d "alert('c');" -s "http://localhost:8089/cubby/data?token=$token")
[ "$message" == "Stashed." ];       ok "reset data stashed"

message=$(curl -s "http://localhost:8089/cubby/verity.js?http://foo.com/login")
[ "$message" == "alert('c');" ];    ok "get reset data # ${message}"

message=$(curl -s "http://localhost:8089/cubby/shutdown?"$key)
[ "$message" == "Goodbye." ];       ok "server said goodbye"

sleep 1
if kill -s 0 $pid 2> /dev/null; then
    kill $pid
    false;
else
    true;
fi
ok "program has been shutdown"
