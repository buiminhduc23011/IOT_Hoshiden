set(COMPONENT_SRCS
	"flag.c"
	"socketioclient_api.cpp"
	"Ethernet.c"
	"ota.c"
	"TCP.c"
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