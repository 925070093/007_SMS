1.修改main.cpp中的passwd为007+的超密，修改user_id为你接收信息的QQ号码  
2.在arm架构设备上运行[g++ -o app main.cpp md5.cpp base64.cpp sha256.cpp -pthread]，编译得到主程序app  
3.007+后台开启adb，用adb push把app和start.sh传输到007+的/home/user目录，把go-cqhttp和config.yml传输到007+的/home/user/go-cqhttp目录  
4.adb shell，用超密登陆，运行[chmod -R 777 /home/user/]给所有文件赋权  
5.运行go-cqhttp，因为windows的原因会乱码无法显示二维码，用adb pull把qrcode.png传输到电脑上再用登陆了机器人的手机QQ扫码  
5.运行[mount -o rw,remount /]解除系统文件写入保护，运行[cp /home/user/start.sh /etc/init.d/start.sh]把启动脚本复制到init.d文件夹下  
6.运行[ln -s /etc/init.d/start.sh /etc/rcS.d/S96start.sh]把启动脚本加入到开机自启项目  
7.重启程序自动运行，此后重启007+程序都会重新自动运行  

Note：不想使用只需运行[rm -rf /etc/rcS.d/S96start.sh]把开机自启关闭后重启即可
