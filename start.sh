#!/bin/sh
cd /home/user/go-cqhttp
nohup /home/user/go-cqhttp/go-cqhttp > /home/user/go-cqhttp/cqhttp_nohup.log 2>&1 &
cd /home/user
nohup /home/user/app > /home/user/app_nohup.log 2>&1 &