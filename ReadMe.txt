1.  目前新增完專案路徑底下的DCMTK「include Path」以及「Library Path」之後，

     會產生「cannot open file "ofstd.lib"」的問題。

     只能先以事先Build在其他地方的路徑代替

============================================================================================

1. 建立專案時，確保「Configuration」->「General」->「Use of MFC」->「Use MFC in a Shared DLL」
    以及「C/C++」->「Code Generation」->「Runtime Library」->「依照Library版本，在此為MDd」

2. DCMTK - 3.6.3
=>  先將Source Code 進行Compile、Build
=>  於專案「Properties」底下：( 新增時只能是當初 Compile的路徑，若是新增另外複製到其他路徑，會有錯誤 )
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