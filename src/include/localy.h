#ifndef __LOCALY__H__
#define __LOCALY__H__

#define IMAGE_DIRECTORY								IMAGEDIR
#define IMAGE_CAPTCHA_DIRECTORY   					IMAGECAPTCHADIR
#define IMAGE_FEED_DIRECTORY 	  					IMAGEFEEDDIR
#define	IMAGE_AVATAR_DIRECTORY						IMAGEAVATARDIR
#define	IMAGE_CHAT_DIRECTORY						IMAGECHATDIR
#define	IMAGE_COMPANIES_DIRECTORY					IMAGECOMPANIESDIR
#define IMAGE_WORK_DIRECTORY						IMAGEWORKDIR
#define IMAGE_BOOK_DIRECTORY    					IMAGEBOOKDIR
#define FONTS_DIRECTORY								FONTSDIR
#define LIST_DIRECTORY								HTMLDIR
#define BROWSER_FILE_PREFIX							"/"	// for instance "\\server\folder" or "http://www.file.serever/com"

#define	DB_NAME										"connme"
#define	DB_LOGIN									"connme"
#define	DB_PASSWORD									"connme"
#define	DB_HOST										"localhost"

#define	LOG_FILE_MAX_LENGTH							500 // user to strip long output to log file
#define	LOG_FILE_NAME								"/home/ikuchin/src_logs/dev.connme.ru.log"
#define	MAIL_FILE_NAME								"/home/ikuchin/src_logs/dev.connme.ru.mail"
#define	MAIL_STDOUT									"/home/ikuchin/src_logs/dev.connme.ru.stdout"
#define	MAIL_STDERR									"/home/ikuchin/src_logs/dev.connme.ru.stderr"
#define	SENDMAIL_FILE_NAME							"/usr/sbin/sendmail"

#define	FALLBACK_LOGGEDIN_USER_DEFAULT_ACTION		"news_feed"
#define	FALLBACK_GUEST_USER_DEFAULT_ACTION			"showmain"
#define	BREADCRUMBS_STR								" | "

#define	DEFAULT_LANGUAGE							"ru"
#define	TEMPLATE_PATH								"templates/"

#define	SMTP_HOST									"127.0.0.1"
#define	SMTP_PORT									25
#define	SMTP_MAILFROM								"noreply@connme.ru"

#define	INDEX_PAGE									1

#define	THREADS_PER_PAGE							20
#define	PHOTO_PER_PAGE								20
#define	MAX_PHOTO									3
#define	NEWS_ON_SINGLE_PAGE							30
#define	FRIENDS_ON_SINGLE_PAGE						30

#define	FALSE										0
#define	TRUE										1

#define	AVATAR_NUMBER_OF_FOLDERS					512
#define	AVATAR_MAX_FILE_SIZE						30 * 1024 * 1024 // --- 1 MegaByte

#define	FEEDIMAGE_NUMBER_OF_FOLDERS					512
#define	FEEDIMAGE_MAX_FILE_SIZE						30 * 1024 * 1024 // --- 1 MegaByte

// --- frequency calling system.cgi?action=EchoRequest
// --- used for presence update and presence caching
#define	FREQUENCY_ECHO_REQUEST						60

// --- chat definitions
#define	CHAT_MAX_MESSAGE_QUEUE						512 - 1 // --- (-1) required to get all "1" in binary representation
#define	CHAT_LOG_FILE_NAME							"/home/ikuchin/src_logs/dev.connme.ru.chat"
#define	CHAT_MAX_PACKET_SIZE						4096
#define CHAT_PRESENCE_CACHE_LIFETIME				FREQUENCY_ECHO_REQUEST
#define	CHAT_MESSAGES_PER_PAGE						30
#define	CHAT_MESSAGES_REQUEST_PER_SEC				3
#define	CHAT_DB_CONNECTION_RESET_TIMEOUT			3600 // --- timeout in sec, resseting db connection
#define CHAT_IMAGE_NUMBER_OF_FOLDERS				512

// --- admin definitions
#define	ADMIN_LOG_FILE_NAME							"/home/ikuchin/src_logs/dev.connme.ru.admin"

#endif
