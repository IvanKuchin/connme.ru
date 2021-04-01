configure_file(number_of_folders.in	"${CONFIG_PROJECT_DIR}/number_of_folders")

install(FILES		"${CONFIG_PROJECT_DIR}/number_of_folders"
	DESTINATION	${CONFIG_INSTALL_DIR}
	PERMISSIONS	OWNER_READ OWNER_WRITE GROUP_READ
)
