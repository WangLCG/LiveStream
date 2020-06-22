代码说明：
  生成onvif 客户端代码架构，支持h265 url获取，通过修改build_all.sh可以生成server端代码

使用环境：
  ubuntu14.04 64位

使用：
 ./build_all.sh 
 最后会在libonvif生成对应的libonvif.a文件，在其它地方使用时拷贝libonvif整个文件夹过去，静态链接libonvif.a
