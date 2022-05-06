GET_DIRECTORY_PROPERTY  (CHAT_PROJECT_DIR DIRECTORY       ${PROJECT_SOURCE_DIR}/src/chat/ DEFINITION CHAT_PROJECT_DIR)
GET_DIRECTORY_PROPERTY  (CHAT_INSTALL_DIR DIRECTORY       ${PROJECT_SOURCE_DIR}/src/chat/ DEFINITION CHAT_INSTALL_DIR)


file(GLOB CONFIG_FILES "*.in")

foreach(config_file ${CONFIG_FILES})
    string(REGEX REPLACE "(^.*/|\\.[^.]*$)" "" file_wo_ext ${config_file})

    configure_file("${file_wo_ext}.in" "${CHAT_PROJECT_DIR}/${file_wo_ext}")

    install(FILES	"${CHAT_PROJECT_DIR}/${file_wo_ext}"
	DESTINATION	${CHAT_INSTALL_DIR}
	COMPONENT	chat
	PERMISSIONS	OWNER_READ OWNER_WRITE GROUP_READ
    )

endforeach()
