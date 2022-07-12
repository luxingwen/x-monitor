#!/bin/bash

echo -e "sha256hmac hello world"
echo -n "hello world" |sha256hmac --key "4a7bc6c59ebc1a83dc38ec4fd537f98994a9210bf09ad9fc8c60c2ae83746d82"

echo -e "\nmd5sum hello world"
echo -n "hello world" |md5sum