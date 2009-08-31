I. For Windows 
	
1. Open the Apache solution and compile it as described here http://httpd.apache.org/docs/2.0/platform/win_compiling.html
2. If you compile Apache successfully you might proceed further
3. Place mod_auth_hard.vcproj, mod_auth_hard.c, mod_auth_aux.cpp, mod_auth_aux.h in modules/aaa folder
4. Compile the project
5. Now you can put mod_auth.so in your Apache modules directory
6. Ensure that the file c:\Program Files\MySQL\MySQL Server 5.1\bin\libmySQL.dll is somewhere on the library search path
7. For configuration use the following parameters:

ModAuth_WaitModifier 2
ModAuth_DiminishTime 60
ModAuth_DiminishModifier 2
ModAuth_DistributedIPs 10
ModAuth_DistributedTime 60
ModAuth_LockoutTime 60
ModAuth_DBUser root
ModAuth_DBPassword 123




II. For Linux

1.Install Apache

./configure --enable-auth=shared
make
make install

2. Create mysql database

3. Compile and install the modifued module

3.1 crate a tmp directory and put mod_auth.c & mod_auth_aux.cpp & mod_auth_auh.h in it
3.2 place the files Makefile and modules.mk in it
3.3 Edit the top lines of the Makefile to meet your Apache isntallation folder
3.4 create and empty .deps file in the tmp directory
3.5 run 'make' to compile (or make CXX=g++)
3.6 run 'make install' to copy the module to /usr/locl/apahce2/modules
3.7 Ensure that 'LoadModule auth_module modules/mod_auth.so' is placed in httpd.conf
3.8 Ensure that libsqlapi.so and libmysqlclient.so are loadable, e.g. they are in /usr/lib
3.9 For configuration use the following parameters:

ModAuth_WaitModifier 2
ModAuth_DiminishTime 60
ModAuth_DiminishModifier 2
ModAuth_DistributedIPs 10
ModAuth_DistributedTime 60
ModAuth_LockoutTime 60
ModAuth_DBUser root
ModAuth_DBPassword 123

