{
  "targets": [
    {
      "target_name": "pixels-please",
      'include_dirs': [
        "<!@(node -p \"require('napi-thread-safe-callback').include\")",
        "<!@(node -p \"require('node-addon-api').include\")",
        "deps"],
      'cflags!': [ '-fno-exceptions', '-D_THREAD_SAFE' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'xcode_settings': {
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'CLANG_CXX_LIBRARY': 'libc++',
        'MACOSX_DEPLOYMENT_TARGET': '10.7',
      },
      'msvs_settings': {
        'VCCLCompilerTool': { 'ExceptionHandling': 1 },
      },
      'conditions': [
        ['OS=="win"', { 'defines': [ '_HAS_EXCEPTIONS=1' ] }]
      ],
      "sources": [
        "src/Threads.cc",
        "src/Pipeline.cc",
        "src/Init.cc"
      ]
    }
  ]
}

