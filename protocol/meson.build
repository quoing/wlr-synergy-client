# wl_protocol_dir = wayland_protos.get_variable(pkgconfig: 'pkgdatadir')

wayland_scanner = find_program('wayland-scanner')

wayland_scanner_code = generator(
        wayland_scanner,
        output: '@BASENAME@-protocol.c',
        arguments: ['private-code', '@INPUT@', '@OUTPUT@'],
)

wayland_scanner_client = generator(
        wayland_scanner,
        output: '@BASENAME@-client-protocol.h',
        arguments: ['client-header', '@INPUT@', '@OUTPUT@'],
)

protocols = [
        'virtual-keyboard-unstable-v1.xml',
        'wlr-virtual-pointer-unstable-v1.xml',
]

client_protos_src = []
client_protos_headers = []
foreach xml : protocols
        client_protos_src += wayland_scanner_code.process(xml)
        client_protos_headers += wayland_scanner_client.process(xml)
endforeach

lib_client_protos = static_library(
        'client_protos',
        client_protos_src + client_protos_headers,
        dependencies: [wayland_client]
)

client_protos = declare_dependency(
        link_with: lib_client_protos,
        sources: client_protos_headers,
)

# vim: set ts=4 sw=4:


