{
  'targets': [
    {
      'target_name': 'ndbus',
      'sources': [
        'src/ndbus.cc',
        'src/ndbus-utils.cc',
        'src/ndbus-connection-setup.cc'
      ],
      'libraries': [
        '<!@(pkg-config glib-2.0 --libs)',
        '<!@(pkg-config dbus-1 --libs)'
      ],
      'include_dirs': [
        '<!@(pkg-config glib-2.0 --cflags-only-I | sed s/-I//g)',
        '<!@(pkg-config dbus-1 --cflags-only-I | sed s/-I//g)'
      ]
    }
  ]
}
