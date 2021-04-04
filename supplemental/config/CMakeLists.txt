configure_file(default_action.in	"${CONFIG_PROJECT_DIR}/default_action")
configure_file(number_of_folders.in	"${CONFIG_PROJECT_DIR}/number_of_folders")
configure_file(max_file_size.in		"${CONFIG_PROJECT_DIR}/max_file_size")


install(FILES		"${CONFIG_PROJECT_DIR}/default_action"
	DESTINATION	${CONFIG_INSTALL_DIR}
	PERMISSIONS	OWNER_READ OWNER_WRITE GROUP_READ
)

install(FILES		"${CONFIG_PROJECT_DIR}/number_of_folders"
	DESTINATION	${CONFIG_INSTALL_DIR}
	PERMISSIONS	OWNER_READ OWNER_WRITE GROUP_READ
)

install(FILES		"${CONFIG_PROJECT_DIR}/max_file_size"
	DESTINATION	${CONFIG_INSTALL_DIR}
	PERMISSIONS	OWNER_READ OWNER_WRITE GROUP_READ
)

