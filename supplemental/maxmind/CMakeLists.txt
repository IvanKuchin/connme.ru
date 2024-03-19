if(NOT ${MAXMIND_DISABLE})
    if(EXISTS "${MAXMIND_PROJECT_DIR}/GeoLite2-City.mmdb")
	message("-- MaxMindDB exists")
    else()
	message("-- Downloading maxmindDB file ...")
        #
        # before cmake v3.7
        #
        file(DOWNLOAD	"https://download.maxmind.com/app/geoip_download?edition_id=GeoLite2-City&license_key=VWPEafipymo0lZIp&suffix=tar.gz"	"${MAXMIND_PROJECT_DIR}/GeoLite2-City.tar.gz")
        #
        # after cmake v3.7
        #
        # file(DOWNLOAD	"https://download.maxmind.com/geoip/databases/GeoLite2-City/download?suffix=tar.gz"	"${MAXMIND_PROJECT_DIR}/GeoLite2-City.tar.gz" USERPWD 158226:VWPEafipymo0lZIp)
	execute_process(
	    COMMAND             tar -zxf GeoLite2-City.tar.gz --wildcards *.mmdb --strip-components 1
	    WORKING_DIRECTORY   ${MAXMIND_PROJECT_DIR}
	    )
	if(EXISTS "${MAXMIND_PROJECT_DIR}/GeoLite2-City.mmdb")
	    MESSAGE("-- MaxMindDB extracted")
	else()
	    MESSAGE("-- ERROR: fail to extract MaxMindDB")
	endif()
    endif()

    install(FILES	"${MAXMIND_PROJECT_DIR}/GeoLite2-City.mmdb" DESTINATION ${MAXMIND_INSTALL_DIR} COMPONENT app)
endif()