TARGET := irssi

LIBS += libc posix libcrypto libssl glib pcre ncurses

IRSSI_DIR := $(call select_from_ports,irssi)/src/app/irssi

INC_DIR += $(IRSSI_DIR)/src
INC_DIR += $(IRSSI_DIR)/src/core
INC_DIR += $(IRSSI_DIR)/src/irc/core
INC_DIR += $(IRSSI_DIR)/src/irc/dcc
INC_DIR += $(IRSSI_DIR)/src/fe-common/core/
INC_DIR += $(PRG_DIR)

CC_OPT += -DHAVE_CONFIG_H
CC_OPT += -DSYSCONFDIR=\"/etc\"
CC_OPT += -DHELPDIR=\"/share/irssi/help\"
CC_OPT += -DTHEMESDIR=\"/share/irssi/themes\"

SRC_C := \
         core/args.c \
         core/channels-setup.c \
         core/channels.c \
         core/chat-commands.c \
         core/chat-protocols.c \
         core/chatnets.c \
         core/commands.c \
         core/core.c \
         core/expandos.c \
         core/ignore.c \
         core/iregex-gregex.c \
         core/levels.c \
         core/line-split.c \
         core/log-away.c \
         core/log.c \
         core/masks.c \
         core/misc.c \
         core/modules-load.c \
         core/modules.c \
         core/net-disconnect.c \
         core/net-nonblock.c \
         core/net-sendbuffer.c \
         core/network-openssl.c \
         core/network.c \
         core/nicklist.c \
         core/nickmatch-cache.c \
         core/pidwait.c \
         core/queries.c \
         core/rawlog.c \
         core/recode.c \
         core/servers-reconnect.c \
         core/servers-setup.c \
         core/servers.c \
         core/session.c \
         core/settings.c \
         core/signals.c \
         core/special-vars.c \
         core/tls.c \
         core/utf8.c \
         core/wcwidth.c \
         core/write-buffer.c \
         fe-common/core/chat-completion.c \
         fe-common/core/command-history.c \
         fe-common/core/completion.c \
         fe-common/core/fe-channels.c \
         fe-common/core/fe-common-core.c \
         fe-common/core/fe-core-commands.c \
         fe-common/core/fe-exec.c \
         fe-common/core/fe-expandos.c \
         fe-common/core/fe-help.c \
         fe-common/core/fe-ignore-messages.c \
         fe-common/core/fe-ignore.c \
         fe-common/core/fe-log.c \
         fe-common/core/fe-messages.c \
         fe-common/core/fe-modules.c \
         fe-common/core/fe-queries.c \
         fe-common/core/fe-recode.c \
         fe-common/core/fe-server.c \
         fe-common/core/fe-settings.c \
         fe-common/core/fe-tls.c \
         fe-common/core/fe-windows.c \
         fe-common/core/formats.c \
         fe-common/core/hilight-text.c \
         fe-common/core/keyboard.c \
         fe-common/core/module-formats.c \
         fe-common/core/printtext.c \
         fe-common/core/themes.c \
         fe-common/core/window-activity.c \
         fe-common/core/window-commands.c \
         fe-common/core/window-items.c \
         fe-common/core/windows-layout.c \
         fe-common/irc/dcc/fe-dcc-chat-messages.c \
         fe-common/irc/dcc/fe-dcc-chat.c \
         fe-common/irc/dcc/fe-dcc-get.c \
         fe-common/irc/dcc/fe-dcc-send.c \
         fe-common/irc/dcc/fe-dcc-server.c \
         fe-common/irc/dcc/fe-dcc.c \
         fe-common/irc/dcc/module-formats.c \
         fe-common/irc/fe-common-irc.c \
         fe-common/irc/fe-ctcp.c \
         fe-common/irc/fe-events-numeric.c \
         fe-common/irc/fe-events.c \
         fe-common/irc/fe-irc-channels.c \
         fe-common/irc/fe-irc-commands.c \
         fe-common/irc/fe-irc-messages.c \
         fe-common/irc/fe-irc-queries.c \
         fe-common/irc/fe-irc-server.c \
         fe-common/irc/fe-ircnet.c \
         fe-common/irc/fe-modes.c \
         fe-common/irc/fe-netjoin.c \
         fe-common/irc/fe-netsplit.c \
         fe-common/irc/fe-sasl.c \
         fe-common/irc/fe-whois.c \
         fe-common/irc/irc-completion.c \
         fe-common/irc/module-formats.c \
         fe-common/irc/notifylist/fe-notifylist.c \
         fe-common/irc/notifylist/module-formats.c \
         fe-text/gui-entry.c \
         fe-text/gui-expandos.c \
         fe-text/gui-printtext.c \
         fe-text/gui-readline.c \
         fe-text/gui-windows.c \
         fe-text/irssi.c \
         fe-text/lastlog.c \
         fe-text/mainwindow-activity.c \
         fe-text/mainwindows-layout.c \
         fe-text/mainwindows.c \
         fe-text/module-formats.c \
         fe-text/statusbar-config.c \
         fe-text/statusbar-items.c \
         fe-text/statusbar.c \
         fe-text/term-terminfo.c \
         fe-text/term.c \
         fe-text/terminfo-core.c \
         fe-text/textbuffer-commands.c \
         fe-text/textbuffer-view.c \
         fe-text/textbuffer.c \
         irc/core/bans.c \
         irc/core/channel-events.c \
         irc/core/channel-rejoin.c \
         irc/core/channels-query.c \
         irc/core/ctcp.c \
         irc/core/irc-cap.c \
         irc/core/irc-channels-setup.c \
         irc/core/irc-channels.c \
         irc/core/irc-chatnets.c \
         irc/core/irc-commands.c \
         irc/core/irc-core.c \
         irc/core/irc-expandos.c \
         irc/core/irc-masks.c \
         irc/core/irc-nicklist.c \
         irc/core/irc-queries.c \
         irc/core/irc-servers-reconnect.c \
         irc/core/irc-servers-setup.c \
         irc/core/irc-servers.c \
         irc/core/irc-session.c \
         irc/core/irc.c \
         irc/core/lag.c \
         irc/core/massjoin.c \
         irc/core/mode-lists.c \
         irc/core/modes.c \
         irc/core/netsplit.c \
         irc/core/sasl.c \
         irc/core/servers-idle.c \
         irc/core/servers-redirect.c \
         irc/dcc/dcc-autoget.c \
         irc/dcc/dcc-chat.c \
         irc/dcc/dcc-get.c \
         irc/dcc/dcc-queue.c \
         irc/dcc/dcc-resume.c \
         irc/dcc/dcc-send.c \
         irc/dcc/dcc-server.c \
         irc/dcc/dcc.c \
         irc/flood/autoignore.c \
         irc/flood/flood.c \
         irc/notifylist/notify-commands.c \
         irc/notifylist/notify-ison.c \
         irc/notifylist/notify-setup.c \
         irc/notifylist/notify-whois.c \
         irc/notifylist/notifylist.c \
         lib-config/get.c \
         lib-config/parse.c \
         lib-config/set.c \
         lib-config/write.c

# generated src files
SRC_C += irc-modules.c \
         irc.c

vpath %.c $(IRSSI_DIR)/src
vpath %.c $(PRG_DIR)
