I. For Windows 
	
1. Open the Apache solution and compile it as described here http://httpd.apache.org/docs/2.0/platform/win_compiling.html
2. If you compile Apache successfully you might proceed further
3. Copy mod_auth_aux.cpp and mod_auth_aux.h in the modules\aaa folder
4. Replace mod_auth.c wih the modified version
5. Right click on mod_auth project and choose "Add New Item", then add mod_auth_aux.cpp and mod_auth_aux.h to the mod_auth project
6. Click mod_auth project properties and:
  6.1 Add "SQLAPI\include\" to the list of Additional Include Directories
  6.2 Add the appropriate SQLAPI library. For static builds these are sqlapisd.lib(debug) and sqlapis.lib(retail). You have to add version.lib because it is requred by SQLAPI
  6.3 Add SQLAPI\lib folder to the list of Additional Library Directories
7. Compile mod_auth/Apache again and the modified mod_auth is compiled as mod_auth.so
8. Now you can put mod_auth.so in your Apache modules directory
9. Ensure that the file c:\Program Files\MySQL\MySQL Server 5.1\bin\libmySQL.dll is somewhere on the library search path
10. For configuration use the following parameters:

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

