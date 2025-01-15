set(COMPONENT_SRCS
	"Application/loopback/loopback.c"
	"W5500/w5500.c"
	"wizchip_conf.c"
	"TCP.c"
	"socket.c"
	"flag.c"
	"socketioclient_api.cpp"
	"ota.c"
	"IO.c"
	"app_main.cpp"
	)

set(COMPONENT_ADD_INCLUDEDIRS
	"."
	"${CMAKE_CURRENT_LIST_DIR}"
	)

idf_component_register(SRCS "${COMPONENT_SRCS}"
					   INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS}
					  )