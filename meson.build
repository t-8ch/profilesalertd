project('profilesalertd', 'c',
  default_options : ['warning_level=3', 'c_std=c11'])

libsystemd = dependency('libsystemd')
systemd = dependency('systemd')

conf_data = configuration_data()
conf_data.set_quoted('PROJECT_NAME', meson.project_name())

configure_file(configuration: conf_data, output : 'config.h')

exe = executable('profilesalertd', 'profilesalertd.c',
  dependencies : [libsystemd],
  install : true)

systemd_units_dir = systemd.get_pkgconfig_variable('systemduserunitdir')

configuration = configuration_data()
configuration.set('bindir', join_paths(get_option('prefix'), get_option('bindir')))

configure_file(
  configuration : configuration,
  input: 'profilesalertd.service.in',
  output: '@BASENAME@', install_dir: systemd_units_dir,
)
