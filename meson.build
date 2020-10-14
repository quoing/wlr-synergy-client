project(
	'wlr-synergy-client',
	'c',
	version: '0.0.1',
	license: 'MIT',
	default_options: [
		'c_std=c11',
		'warning_level=2',
		'werror=true',
	],
)

add_global_arguments([
                      '-Wno-pointer-sign',
                      '-Wno-deprecated-declarations',
                      '-Wno-unused',
                      '-Wno-incompatible-pointer-types',
                      '-Wno-discarded-qualifiers'],
                     language: 'c')

cc = meson.get_compiler('c')
#wayland_client = cc.find_library('wayland-client', required: false)
#wayland_client = cc.find_library('wayland-client', required: false)
#math = cc.find_library('m', required:false)
math = cc.find_library('m')
xkbcommon = dependency('xkbcommon')
wayland_client = dependency('wayland-client')

includes = include_directories('include')

subdir('protocol')

uSynergy = library('synergy-micro-client', 'synergy-micro-client/uSynergy.c')

src_files = [
	'wlr-synergy-client.c',
	#'ascii_raw_keymap.c',
	]

executable(
	'wlr-synergy-client', 
	files(src_files),
	link_with: [uSynergy],
	include_directories: [includes],
	dependencies: [
	  client_protos,
	  math, 
	  wayland_client,
	  xkbcommon,
	],
)

