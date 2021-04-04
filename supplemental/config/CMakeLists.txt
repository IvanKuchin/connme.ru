configure_file(default_action.in	"${CONFIG_PROJECT_DIR}/default_action")
configure_file(number_of_folders.in	"${CONFIG_PROJECT_DIR}/number_of_folders")
configure_file(max_file_size.in		"${CONFIG_PROJECT_DIR}/max_file_size")
configure_file(image_max_height.in	"${CONFIG_PROJECT_DIR}/image_max_height")
configure_file(image_max_width.in	"${CONFIG_PROJECT_DIR}/image_max_width")


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

install(FILES		"${CONFIG_PROJECT_DIR}/image_max_width"
	DESTINATION	${CONFIG_INSTALL_DIR}
	PERMISSIONS	OWNER_READ OWNER_WRITE GROUP_READ
)

install(FILES		"${CONFIG_PROJECT_DIR}/image_max_height"
	DESTINATION	${CONFIG_INSTALL_DIR}
	PERMISSIONS	OWNER_READ OWNER_WRITE GROUP_READ
)
