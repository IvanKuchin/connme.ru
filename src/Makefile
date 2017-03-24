G++						=	g++
GCC						=	gcc
ROOTDIR					=	/home/httpd/dev.connme.ru
CGIDIR					=	$(ROOTDIR)/cgi-bin
HTMLDIR					=	$(ROOTDIR)/html/
IMAGEDIR				=	$(ROOTDIR)/html/images/
IMAGECAPTCHADIR			=	$(IMAGEDIR)captcha/
IMAGEAVATARDIR			=	$(IMAGEDIR)avatars
IMAGECHATDIR			=	$(IMAGEDIR)chat
IMAGECOMPANIESDIR		=	$(IMAGEDIR)companies
IMAGEFEEDDIR			=   $(IMAGEDIR)feed
IMAGEWORKDIR			=   $(IMAGEDIR)works/
IMAGEBOOKDIR			=   $(IMAGEDIR)
FONTSDIR				=	$(ROOTDIR)/html/fonts/
CGIADMINDIR 			= 	$(CGIDIR)/admin
TEMPLATEDIR 			=	$(CGIDIR)/templates
TEMPLATEADMINDIR 		=	$(CGIDIR)/admin/templates
LIBWEBSOCKET_DATADIR 	=	/usr/local/share

DEFINITIONS 			= 	-DMYSQL_4 -DIMAGECAPTCHADIR=\"$(IMAGECAPTCHADIR)\" -DHTACCESSDIR=\"$(HTMLDIR)\" -DLOCKDIR=\"$(CGIDIR)/lock\" -DIMAGEFEEDDIR=\"$(IMAGEFEEDDIR)\"  \
							-DIMAGEAVATARDIR=\"$(IMAGEAVATARDIR)\" -DIMAGEBOOKDIR=\"$(IMAGEBOOKDIR)\" -DIMAGEWORKDIR=\"$(IMAGEWORKDIR)\" -DIMAGEDIR=\"$(IMAGEDIR)\" \
							-DFONTSDIR=\"$(FONTSDIR)\" -DHTMLDIR=\"$(HTMLDIR)\" -DMAXMIND_ACTIVATE -DIMAGEMAGICK_ACTIVATE -DLINUX -DLIBWEBSOCKET_DATADIR=\"$(LIBWEBSOCKET_DATADIR)\"
INCDIR					=	-I ./include -I /usr/local/include/mysql -I /usr/local/include -I /usr/include/mysql `Magick-config --cflags`
# -pipe - using all cores 
CPPFLAGS  				=	$(DEFINITIONS) -std=c++1y -Wall -g -ggdb -fno-inline -pipe $(INCDIR)
CFLAGS  				=	$(DEFINITIONS) -Wall -g -ggdb $(INCDIR)

DYNAMIC_LIBS 			=	-lmysqlclient -lmaxminddb
STATIC_LIBS 			= 
LDFLAGS					=	-Wl,-Bstatic $(STATIC_LIBS) -Wl,-Bdynamic $(DYNAMIC_LIBS) 
COMMON_OBJS 			=	ccgi.o cvars.o cmysql.o crequest.o ccookie.o cexception.o cmail.o clock.o ctemplate.o cactivator.o csession.o cmenu.o cforum.o cfiles.o cuser.o ccatalog.o cstatistics.o

all: index.cgi avataruploader.cgi imageuploader.cgi system.cgi admin.cgi chat-server

index.cgi: index.o $(COMMON_OBJS)
	$(G++) -o $(CGIDIR)/index.cgi index.o $(COMMON_OBJS) \
	$(LDFLAGS) `Magick++-config --ldflags` `Magick++-config --libs`

avataruploader.cgi: avataruploader.o  $(COMMON_OBJS)
	$(G++) -o $(CGIDIR)/avataruploader.cgi  avataruploader.o $(COMMON_OBJS) \
	$(LDFLAGS) `Magick++-config --ldflags` `Magick++-config --libs`

imageuploader.cgi: imageuploader.o $(COMMON_OBJS) 
	$(G++) -o $(CGIDIR)/imageuploader.cgi imageuploader.o $(COMMON_OBJS) \
	$(LDFLAGS) `Magick++-config --ldflags` `Magick++-config --libs`

system.cgi: system.o $(COMMON_OBJS) 
	$(G++) -o $(CGIDIR)/system.cgi system.o $(COMMON_OBJS) \
	$(LDFLAGS) 

chat-server: chat-server.o chat-server-protocol-dumb.o chat-server-protocol-message.o chat-server-protocol-status.o chat-server-jsonparser.o \
				chat-server-singlemessage.o chat-server-presencecache.o chat-server-requestratelimiter.o $(COMMON_OBJS)
	$(G++) -o chat-server chat-server.o chat-server-protocol-dumb.o  chat-server-protocol-message.o chat-server-protocol-status.o chat-server-jsonparser.o \
				chat-server-singlemessage.o chat-server-presencecache.o chat-server-requestratelimiter.o $(COMMON_OBJS) \
	$(LDFLAGS) -lpthread -lwebsockets -lz -lssl -lcrypto -lm

admin.cgi: admin.o  $(COMMON_OBJS) 
	$(G++) -o $(CGIADMINDIR)/admin.cgi admin.o $(COMMON_OBJS) \
	$(LDFLAGS) 

# resource.cgi: resource.o ccgi.o cvars.o cmysql.o ccookie.o crequest.o clock.o cfiles.o cmail.o cexception.o ctemplate.o
# 	$(G++) -o $(CGIADMINDIR)/resource.cgi resource.o ccgi.o cvars.o cmysql.o ccookie.o crequest.o clock.o cfiles.o \
# 	cexception.o cmail.o ctemplate.o csession.o \
# 	$(LDFLAGS) `Magick-config --ldflags` `Magick-config --libs`

# parts.cgi: parts.o ccgi.o cvars.o cmysql.o ccookie.o crequest.o clock.o cmenu.o cexception.o cforum.o cfiles.o cmail.o \
# 	ctemplate.o ccatalog.o
# 	$(G++) -o $(CGIADMINDIR)/parts.cgi parts.o ccgi.o cvars.o cmysql.o ccookie.o crequest.o cexception.o \
# 	clock.o cmenu.o cforum.o cfiles.o cmail.o ctemplate.o ccatalog.o csession.o \
# 	$(LDFLAGS) `Magick-config --ldflags` `Magick-config --libs`

index.o: index.cpp include/cvars.h include/ccgi.h include/localy.h include/cmail.h include/clog.h include/ccookie.h include/crequest.h \
				include/cmysql.h  include/cexception.h include/clock.h include/cactivator.h include/ccatalog.h include/utilities.h

avataruploader.o: avataruploader.cpp include/avataruploader.h include/cvars.h include/ccgi.h include/localy.h include/cmail.h include/clog.h include/ccookie.h include/crequest.h \
				include/cmysql.h  include/cexception.h include/clock.h include/cactivator.h include/ccatalog.h include/utilities.h

imageuploader.o: imageuploader.cpp include/imageuploader.h include/cvars.h include/ccgi.h include/localy.h include/cmail.h include/clog.h include/ccookie.h include/crequest.h \
				include/cmysql.h  include/cexception.h include/clock.h include/cactivator.h include/ccatalog.h include/utilities.h

system.o: system.cpp include/cvars.h include/ccgi.h include/localy.h include/cmail.h include/clog.h include/ccookie.h include/crequest.h \
				include/cmysql.h  include/cexception.h include/clock.h include/cactivator.h include/ccatalog.h include/utilities.h

chat-server.o chat-server-protocol-status.o chat-server-protocol-message.o: chat-server.cpp chat-server-protocol-dumb.cpp chat-server-protocol-message.cpp chat-server-protocol-status.cpp \
				include/chat-server.h include/chat-server-protocol-message.h include/chat-server-protocol-status.h include/chat-server-protocol-dumb.h \
				include/chat-server-jsonparser.h include/chat-server-singlemessage.h include/chat-server-presencecache.h include/chat-server-requestratelimiter.h \
				include/cvars.h include/ccgi.h include/localy.h include/cmail.h include/clog.h include/ccookie.h include/crequest.h \
				include/cmysql.h  include/cexception.h include/clock.h include/cactivator.h include/ccatalog.h include/utilities.h
	$(G++) -c chat-server.cpp chat-server-protocol-dumb.cpp chat-server-protocol-message.cpp chat-server-protocol-status.cpp \
	$(CPPFLAGS)

chat-server-jsonparser.o: chat-server-jsonparser.cpp include/chat-server-jsonparser.h include/clog.h
chat-server-singlemessage.o: chat-server-singlemessage.cpp include/chat-server-singlemessage.h include/clog.h
chat-server-presencecache.o: chat-server-presencecache.cpp include/chat-server-presencecache.h include/clog.h include/localy.h include/cmysql.h 
chat-server-requestratelimiter.o: chat-server-requestratelimiter.cpp include/chat-server-requestratelimiter.h include/clog.h include/localy.h include/cmysql.h 


admin.o: admin.cpp include/admin.h include/cvars.h include/ccgi.h include/localy.h include/cmail.h include/clog.h include/ccookie.h include/crequest.h \
				include/cmysql.h  include/cexception.h include/clock.h include/cactivator.h include/ccatalog.h include/utilities.h

cmenu.o: cmenu.cpp include/cmenu.h include/cvars.h include/ccgi.h include/localy.h include/clock.h
ccatalog.o: ccatalog.cpp include/ccatalog.h include/cvars.h include/ccgi.h include/localy.h include/clock.h
cforum.o: cforum.cpp include/cforum.h include/cvars.h include/ccgi.h include/localy.h

# cresource.o: resource.cpp include/cvars.h include/ccgi.h include/localy.h include/clock.h
# parts.o: parts.cpp include/cvars.h include/ccgi.h include/localy.h include/clock.h include/cmenu.h include/ccatalog.h

cuser.o: cuser.cpp include/cmysql.h include/localy.h include/cvars.h
cactivator.o: cactivator.cpp include/cactivator.h include/cmysql.h include/localy.h include/cexception.h include/ccgi.h
csession.o: csession.cpp include/csession.h include/cmysql.h include/localy.h include/cexception.h include/ccgi.h

cfiles.o: cfiles.cpp include/cfiles.h include/clog.h include/localy.h include/cexception.h
ctemplate.o: ctemplate.cpp include/localy.h include/cvars.h include/ctemplate.h include/clog.h include/cexception.h
clock.o: clock.cpp include/localy.h include/clock.h include/cexception.h
cmail.o: cmail.cpp include/localy.h include/cmail.h include/clog.h include/cmysql.h include/ctemplate.h include/cvars.h
cexception.o:	cexception.cpp include/cmysql.h include/localy.h
ccgi.o:	ccgi.cpp include/ccgi.h include/clog.h include/crequest.h include/cexception.h include/csession.h
cvars.o: cvars.cpp include/cvars.h include/clog.h include/localy.h include/cexception.h
cmysql.o: cmysql.cpp include/cmysql.h include/clog.h include/localy.h include/cexception.h
crequest.o: crequest.cpp include/crequest.h include/cvars.h include/clog.h include/localy.h
ccookie.o: ccookie.cpp include/ccookie.h include/clog.h include/localy.h include/cexception.h
cstatistics.o: cstatistics.cpp include/cstatistics.h include/clog.h

install:
	cp -f ./chat-server /usr/local/sbin/
	cp -f ./etc/init.d/chatd /etc/init.d/

uninstall:
	rm -f $(CGIDIR)/*.cgi
	rm -f /usr/local/sbin/chat-server
	rm -f /etc/init.d/chatd

clean:
	rm -f ./*.o
	rm -f ./*.cgi
	rm -f ./chat-server
