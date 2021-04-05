file(GLOB CONFIG_FILES "*.in")

foreach(config_file ${CONFIG_FILES})
    string(REGEX REPLACE "(^.*/|\\.[^.]*$)" "" file_wo_ext ${config_file})


    message("-----" ${file_wo_ext})

    configure_file("${file_wo_ext}.in" "${CONFIG_PROJECT_DIR}/${file_wo_ext}")

    install(FILES	"${CONFIG_PROJECT_DIR}/${file_wo_ext}"
	DESTINATION	${CONFIG_INSTALL_DIR}
	PERMISSIONS	OWNER_READ OWNER_WRITE GROUP_READ
    )

endforeach()

# configure_file(default_action.in	"${CONFIG_PROJECT_DIR}/default_action")

#install(FILES		"${CONFIG_PROJECT_DIR}/default_action"
#	DESTINATION	${CONFIG_INSTALL_DIR}
#	PERMISSIONS	OWNER_READ OWNER_WRITE GROUP_READ
#)

