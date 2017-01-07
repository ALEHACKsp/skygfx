workspace "skygfx"
   configurations { "Release", "Debug" }
   location "build"
   
   files { "external/*.*" }
   files { "resources/*.*" }
   files { "shaders/*.*" }
   files { "source/*.*" }
   
   includedirs { "external" }
   includedirs { "resources" }
   includedirs { "shaders" }
   includedirs { "source" }
   includedirs { os.getenv("RWSDK36") }
   
   prebuildcommands {
	"for /R \"../shaders/ps/\" %%f in (*.hlsl) do \"%DXSDK_DIR%/Utilities/bin/x86/fxc.exe\" /T ps_2_0 /nologo /E main /Fo ../resources/cso/%%~nf.cso %%f",
	"for /R \"../shaders/vs/\" %%f in (*.hlsl) do \"%DXSDK_DIR%/Utilities/bin/x86/fxc.exe\" /T vs_2_0 /nologo /E main /Fo ../resources/cso/%%~nf.cso %%f",
	"\"%DXSDK_DIR%/Utilities/bin/x86/fxc.exe\" /T ps_2_0 /nologo /E mainPS /Fo ../resources/cso/vcsReflPS.cso ../shaders/pvs/vcsReflPS.hlsl",
	"\"%DXSDK_DIR%/Utilities/bin/x86/fxc.exe\" /T vs_2_0 /nologo /E mainVS /Fo ../resources/cso/vcsReflVS.cso ../shaders/pvs/vcsReflVS.hlsl",
	"\"%DXSDK_DIR%/Utilities/bin/x86/fxc.exe\" /T ps_2_0 /nologo /E mainPS /Fo ../resources/cso/vehiclePS.cso ../shaders/pvs/vehiclePS.hlsl",
	"\"%DXSDK_DIR%/Utilities/bin/x86/fxc.exe\" /T vs_2_0 /nologo /E mainVS /Fo ../resources/cso/vehicleVS.cso ../shaders/pvs/vehicleVS.hlsl"
   }
	  
project "skygfx"
   kind "SharedLib"
   language "C++"
   targetdir "bin/%{cfg.buildcfg}"
   targetextension ".asi"
   characterset ("MBCS")

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
	  flags { "StaticRuntime" }
	  