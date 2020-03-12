1.  目前新增完專案路徑底下的DCMTK「include Path」以及「Library Path」之後，

     會產生「cannot open file "ofstd.lib"」的問題。

     只能先以事先Build在其他地方的路徑代替

============================================================================================

1. 建立專案時，確保「Configuration」->「General」->「Use of MFC」->「Use MFC in a Shared DLL」
	、「Configuration」->「General」->「Character Set」->「Use Multi-Byte Character Set」
	、「C/C++」->「Code Generation」->「Runtime Library」->「依照Library版本，在此為MDd」

2. DCMTK - 3.6.3
=>  先將Source Code 進行Compile、Build
=>  於專案「Properties」底下：（善用Macros => $SolutionDir）
       「C/C++」->「General」->「Additional Include Directories」新增 Compile 我們 Source Code 的路徑底下的 include 資料夾
       「Linker」->「General」->「Additional Library Directories」新增 Compile 我們 Source Code 的路徑底下的 lib 資料夾以及支援庫的zlib底下的 lib 資料夾
       「Linker」->「Input」->「Additional Dependencies」依序新增 ：
Iphlpapi.lib
ws2_32.lib
wsock32.lib
netapi32.lib
ofstd.lib
oflog.lib
dcmdata.lib
dcmdsig.lib
dcmnet.lib
dcmsr.lib
dcmimgle.lib
dcmqrdb.lib
dcmtls.lib
dcmwlm.lib
dcmpstat.lib
dcmjpls.lib
dcmjpeg.lib
dcmimage.lib
charls.lib
ijg8.lib
ijg12.lib
ijg16.lib
i2d.lib
zlib_d.lib

3. FreeGLUT 3.0.0
=> 至官網下載已經編譯好的文件：Prepackaged Releases -> Martin's Windows binaries (MSVC and MinGW) -> Download freeglut3.0.0 for MinGW

4. GLEW 
=> 至官網下載已經編譯好的文件：Binaries-> Windows 32-bit and 64 bit

5. 記得在「Build Events」->「Post-Build Event」：
copy "$(SolutionDir)Library\openGL\FreeGLUT\bin\$(PlatformTarget)\*.dll" "$(OutputPath)" /Y
copy "$(SolutionDir)Library\openGL\GLEW\bin\Release\$(PlatformTarget)\*.dll" "$(OutputPath)" /Y


