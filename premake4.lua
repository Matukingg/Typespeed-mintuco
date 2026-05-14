solution "Typespeed"
   configurations { "Debug", "Release" }
   location "build"
   language "C++"

project "typespeed"
   kind "ConsoleApp"
   language "C++"
   files { "src/**.hpp", "src/**.cpp" }
   includedirs {
      "src",
      "src/core",
      "src/graphics",
      "src/input",
      "src/game",
      "src/textbank",
      "src/stats",
      "src/globalstats",
   }
   links {
      "allegro",
      "allegro_image",
      "allegro_font",
      "allegro_ttf",
      "allegro_primitives",
   }
   configuration "Debug"
      defines { "DEBUG" }
      flags { "Symbols" }
      targetdir "build/bin/debug"
      objdir    "build/obj/debug"
   configuration "Release"
      defines { "NDEBUG" }
      flags { "Optimize" }
      targetdir "build/bin/release"
      objdir    "build/obj/release"
