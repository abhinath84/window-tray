{
  "targets": [{
    "target_name": "tray",
    "include_dirs" : [
      "<!@(node -p \"require('node-addon-api').include\")",
      "src",
    ],
    "sources": [
      "cpp/utils/n-utils.h",
      "cpp/utils/win-utils.h",
      "cpp/utils/node_async_call.h",
      "cpp/utils/node_async_call.cc",
      "cpp/tray.h",
      "cpp/tray.cc",
      "cpp/main.cc",
    ],
    "defines": [ 
	"NAPI_DISABLE_CPP_EXCEPTIONS",
	"UNICODE"
    ],
    "cflags!": [
        "-fno-exceptions"
    ],
    "cflags_cc!": [
        "-fno-exceptions"
    ],
    "conditions": [
      [
        "OS=='win'", {
          "defines": [
              "_UNICODE",
              "_WIN32_WINNT=0x0601"
            ],
          "configurations": {
            "Release": {
              "msvs_settings": {
                "VCCLCompilerTool" : {
                  "ExceptionHandling": 1,
                }
              }
            },
            "Debug": {
              "msvs_settings": {
                "VCCLCompilerTool" : {
                  "ExceptionHandling": 1,
                }
              }
            }
          }
        }
      ]
    ]
  }]
}