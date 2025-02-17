/*
linphone
Copyright (C) 2000  Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef LINPHONECORE_H
#define LINPHONECORE_H


#include <osipparser2/osip_message.h>
#include <ortp/ortp.h>
#include <ortp/payloadtype.h>
#include <mediastreamer2/mscommon.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _MSSndCard;
struct _LinphoneCore;

bool_t payload_type_enabled(struct _PayloadType *pt);
bool_t payload_type_check_usable(struct _PayloadType *pt, const struct _LinphoneCore *lc);
void payload_type_set_enable(struct _PayloadType *pt,int value);
const char *payload_type_get_description(struct _PayloadType *pt);
int payload_type_get_bitrate(PayloadType *pt);
const char *payload_type_get_mime(PayloadType *pt);
int payload_type_get_rate(PayloadType *pt);


struct _LpConfig;

typedef struct sip_config
{
	char *contact;
	char *guessed_contact;
	int sip_port;
	MSList *proxies;
	MSList *deleted_proxies;
	int inc_timeout;	/*timeout after an un-answered incoming call is rejected*/
	bool_t use_info;
	bool_t guess_hostname;
	bool_t loopback_only;
	bool_t ipv6_enabled;
	bool_t sdp_200_ack;
} sip_config_t;

typedef struct rtp_config
{
	int audio_rtp_port;
	int video_rtp_port;
	int audio_jitt_comp;  /*jitter compensation*/
	int video_jitt_comp;  /*jitter compensation*/
}rtp_config_t;



typedef struct net_config
{
	char *nat_address;
	char *stun_server;
	int download_bw;
	int upload_bw;
	int firewall_policy;
	bool_t nat_sdp_only;
}net_config_t;


typedef struct sound_config
{
	struct _MSSndCard * ring_sndcard;	/* the playback sndcard currently used */
	struct _MSSndCard * play_sndcard;	/* the playback sndcard currently used */
	struct _MSSndCard * capt_sndcard; /* the capture sndcard currently used */
	const char **cards;
	int latency;	/* latency in samples of the current used sound device */
	char rec_lev;
	char play_lev;
	char ring_lev;
	char source;
	char pad;
	char *local_ring;
	char *remote_ring;
	bool_t ec;
} sound_config_t;

typedef struct codecs_config
{
	MSList *audio_codecs;  /* list of audio codecs in order of preference*/
	MSList *video_codecs;	/* for later use*/
}codecs_config_t;

typedef struct video_config{
	char *device;
	bool_t capture;
	bool_t show_local;
	bool_t display;
}video_config_t;

typedef struct ui_config
{
	int is_daemon;
	int is_applet;
	unsigned int timer_id;  /* the timer id for registration */
}ui_config_t;



typedef struct autoreplier_config
{
	int enabled;
	int after_seconds;		/* accept the call after x seconds*/
	int max_users;			/* maximum number of user that can call simultaneously */
	int max_rec_time;  	/* the max time of incoming voice recorded */
	int max_rec_msg;		/* maximum number of recorded messages */
	const char *message;		/* the path of the file to be played */
}autoreplier_config_t;


struct _LinphoneCore;
struct _sdp_context;
	
typedef struct _StreamParams
{
	int initialized;
	int line;
	int localport;
	int remoteport;
	char *remoteaddr;
	int pt;
} StreamParams;

typedef enum _LCState{
	LCStateInit,
	LCStateRinging,
	LCStateAVRunning
}LCState;

typedef enum _LinphoneCallDir {LinphoneCallOutgoing, LinphoneCallIncoming} LinphoneCallDir;


typedef struct _LinphoneCall
{
	struct _LinphoneCore *core;
	StreamParams audio_params;
	StreamParams video_params;
	LinphoneCallDir dir;
	struct _RtpProfile *profile;	/*points to the local_profile or to the remote "guessed" profile*/
	struct _LinphoneCallLog *log;
	int cid; /*call id */
	int did; /*dialog id */
	int tid; /*last transaction id*/
	struct _sdp_context *sdpctx;
	time_t start_time;
	LCState	state;
	bool_t auth_pending;	
} LinphoneCall;

LinphoneCall * linphone_call_new_outgoing(struct _LinphoneCore *lc, const osip_from_t *from, const osip_to_t *to);
LinphoneCall * linphone_call_new_incoming(struct _LinphoneCore *lc, const char *from, const char *to, int cid, int did , int tid);
#define linphone_call_set_state(lcall,st)	(lcall)->state=(st)
void linphone_call_destroy(struct _LinphoneCall *obj);


typedef enum _LinphoneCallStatus { 
	LinphoneCallSuccess,
	LinphoneCallAborted,
	LinphoneCallMissed
} LinphoneCallStatus;

typedef struct _LinphoneCallLog{
	LinphoneCallDir dir;
	LinphoneCallStatus status;
	char *from;
	char *to;
	char start_date[128];
	int duration;
	
} LinphoneCallLog;

/* private: */
LinphoneCallLog * linphone_call_log_new(LinphoneCall *call, char *local, char * remote);
void linphone_call_log_completed(LinphoneCallLog *calllog, LinphoneCall *call);
void linphone_call_log_destroy(LinphoneCallLog *cl);

/*public: */
char * linphone_call_log_to_str(LinphoneCallLog *cl);

typedef enum{
	LinphoneSPWait,
	LinphoneSPDeny,
	LinphoneSPAccept
}LinphoneSubscribePolicy;

typedef struct _LinphoneFriend{
	osip_from_t *url;
	int in_did;
	int out_did;
	int sid;
	int nid;
	LinphoneSubscribePolicy pol;
	struct _LinphoneProxyConfig *proxy;
	struct _LinphoneCore *lc;
	bool_t subscribe;
	bool_t inc_subscribe_pending;
}LinphoneFriend;	

LinphoneFriend * linphone_friend_new();
LinphoneFriend *linphone_friend_new_with_addr(const char *addr);
int linphone_friend_set_sip_addr(LinphoneFriend *fr, const char *name);
int linphone_friend_send_subscribe(LinphoneFriend *fr, bool_t val);
int linphone_friend_set_inc_subscribe_policy(LinphoneFriend *fr, LinphoneSubscribePolicy pol);
int linphone_friend_set_proxy(LinphoneFriend *fr, struct _LinphoneProxyConfig *cfg);
void linphone_friend_edit(LinphoneFriend *fr);
void linphone_friend_done(LinphoneFriend *fr);
void linphone_friend_destroy(LinphoneFriend *lf);
/* memory returned by those 3 functions must be freed */
char *linphone_friend_get_name(LinphoneFriend *lf);
char *linphone_friend_get_addr(LinphoneFriend *lf);
char *linphone_friend_get_url(LinphoneFriend *lf);	/* name <sip address> */
#define linphone_friend_url(lf) ((lf)->url)

void linphone_friend_write_to_config_file(struct _LpConfig *config, LinphoneFriend *lf, int index);
LinphoneFriend * linphone_friend_new_from_config_file(struct _LinphoneCore *lc, int index);

typedef struct _LinphoneProxyConfig
{
	struct _LinphoneCore *lc;
	char *reg_proxy;
	char *reg_identity;
	char *reg_route;
	char *realm;
	int expires;
	int reg_time;
	int rid;
	bool_t frozen;
	bool_t reg_sendregister;
	bool_t auth_pending;
	bool_t registered;
	bool_t publish;
} LinphoneProxyConfig;

LinphoneProxyConfig *linphone_proxy_config_new(const char *server_addr);
int linphone_proxy_config_set_server_addr(LinphoneProxyConfig *obj, const char *server_addr);
void linphone_proxy_config_set_identity(LinphoneProxyConfig *obj, const char *identity);
void linphone_proxy_config_set_route(LinphoneProxyConfig *obj, const char *route);
void linphone_proxy_config_expires(LinphoneProxyConfig *obj, const int expires);
void linphone_proxy_config_enableregister(LinphoneProxyConfig *obj, bool_t val);
void linphone_proxy_config_edit(LinphoneProxyConfig *obj);
void linphone_proxy_config_done(LinphoneProxyConfig *obj);
void linphone_proxy_config_enable_publish(LinphoneProxyConfig *obj, bool_t val);
#define linphone_proxy_config_get_route(obj)  ((obj)->reg_route)
#define linphone_proxy_config_get_identity(obj)	((obj)->reg_identity)
#define linphone_proxy_config_publish_enabled(obj) ((obj)->publish)
#define linphone_proxy_config_get_addr(obj) ((obj)->reg_proxy)

/* destruction is called automatically when removing the proxy config */
void linphone_proxy_config_destroy(LinphoneProxyConfig *cfg);
LinphoneProxyConfig *linphone_proxy_config_new_from_config_file(struct _LpConfig *config, int index);
void linphone_proxy_config_write_to_config_file(struct _LpConfig* config,LinphoneProxyConfig *obj, int index);

typedef struct _LinphoneAuthInfo
{
	char *username;
	char *realm;
	char *userid;
	char *passwd;
	char *ha1;
	bool_t works;
	bool_t first_time;
}LinphoneAuthInfo;

LinphoneAuthInfo *linphone_auth_info_new(const char *username, const char *userid,
				   										const char *passwd, const char *ha1,const char *realm);
void linphone_auth_info_set_passwd(LinphoneAuthInfo *info, const char *passwd);
/* you don't need those function*/
void linphone_auth_info_destroy(LinphoneAuthInfo *info);
LinphoneAuthInfo * linphone_auth_info_new_from_config_file(struct _LpConfig *config, int pos);

struct _LinphoneChatRoom{
	struct _LinphoneCore *lc;
	char  *peer;
	char *route;
	osip_from_t *peer_url;
	void * user_data;
};
typedef struct _LinphoneChatRoom LinphoneChatRoom;

LinphoneChatRoom * linphone_core_create_chat_room(struct _LinphoneCore *lc, const char *to);
void linphone_chat_room_send_message(LinphoneChatRoom *cr, const char *msg);
void linphone_chat_room_destroy(LinphoneChatRoom *cr);
void linphone_chat_room_set_user_data(LinphoneChatRoom *cr, void * ud);
void * linphone_chat_room_get_user_data(LinphoneChatRoom *cr);

/* describes the different groups of states */
typedef enum _gstate_group {
  GSTATE_GROUP_POWER,
  GSTATE_GROUP_REG,
  GSTATE_GROUP_CALL
} gstate_group_t;

typedef enum _gstate {
  /* states for GSTATE_GROUP_POWER */
  GSTATE_POWER_OFF = 0,        /* initial state */
  GSTATE_POWER_STARTUP,
  GSTATE_POWER_ON,
  GSTATE_POWER_SHUTDOWN,
  /* states for GSTATE_GROUP_REG */
  GSTATE_REG_NONE = 10,       /* initial state */
  GSTATE_REG_OK,
  GSTATE_REG_FAILED,
  /* states for GSTATE_GROUP_CALL */
  GSTATE_CALL_IDLE = 20,      /* initial state */
  GSTATE_CALL_OUT_INVITE,
  GSTATE_CALL_OUT_CONNECTED,
  GSTATE_CALL_IN_INVITE,
  GSTATE_CALL_IN_CONNECTED,
  GSTATE_CALL_END,
  GSTATE_CALL_ERROR
} gstate_t;

struct _LinphoneGeneralState {
  gstate_t old_state;
  gstate_t new_state;
  gstate_group_t group;
  const char *message;
};
typedef struct _LinphoneGeneralState LinphoneGeneralState;

/* retrieve the current state of the specified state group */
gstate_t gstate_get_state(gstate_group_t group);

/* private: set the initial states */
void gstate_initialize(void);

/* private: set a new state */
void gstate_new_state(struct _LinphoneCore *lc, gstate_t new_state, const char *message);

typedef void (*ShowInterfaceCb)(struct _LinphoneCore *lc);
typedef void (*InviteReceivedCb)(struct _LinphoneCore *lc, const char *from);
typedef void (*ByeReceivedCb)(struct _LinphoneCore *lc, const char *from);
typedef void (*DisplayStatusCb)(struct _LinphoneCore *lc, const char *message);
typedef void (*DisplayMessageCb)(struct _LinphoneCore *lc, const char *message);
typedef void (*DisplayUrlCb)(struct _LinphoneCore *lc, const char *message, const char *url);
typedef void (*DisplayQuestionCb)(struct _LinphoneCore *lc, const char *message);
typedef void (*LinphoneCoreCbFunc)(struct _LinphoneCore *lc,void * user_data);
typedef void (*NotifyReceivedCb)(struct _LinphoneCore *lc, LinphoneFriend * fid, const char *url, const char *status, const char *img);
typedef void (*NewUnknownSubscriberCb)(struct _LinphoneCore *lc, LinphoneFriend *lf, const char *url);
typedef void (*AuthInfoRequested)(struct _LinphoneCore *lc, const char *realm, const char *username);
typedef void (*CallLogUpdated)(struct _LinphoneCore *lc, struct _LinphoneCallLog *newcl);
typedef void (*TextMessageReceived)(struct _LinphoneCore *lc, LinphoneChatRoom *room, const char *from, const char *message);
typedef void (*GeneralStateChange)(struct _LinphoneCore *lc, LinphoneGeneralState *gstate);

typedef struct _LinphoneVTable
{
	ShowInterfaceCb show;
	InviteReceivedCb inv_recv;
	ByeReceivedCb bye_recv;
	NotifyReceivedCb notify_recv;
	NewUnknownSubscriberCb new_unknown_subscriber;
	AuthInfoRequested auth_info_requested;
	DisplayStatusCb display_status;
	DisplayMessageCb display_message;
#ifdef VINCENT_MAURY_RSVP
	/* the yes/no dialog box */
	DisplayMessageCb display_yes_no;
#endif
	DisplayMessageCb display_warning;
	DisplayUrlCb display_url;
	DisplayQuestionCb display_question;
	CallLogUpdated call_log_updated;
	TextMessageReceived text_received;
	GeneralStateChange general_state;
} LinphoneCoreVTable;

typedef struct _LCCallbackObj
{
  LinphoneCoreCbFunc _func;
  void * _user_data;
}LCCallbackObj;

typedef enum _LinphoneOnlineStatus{
	LINPHONE_STATUS_UNKNOWN,
	LINPHONE_STATUS_ONLINE,
	LINPHONE_STATUS_BUSY,
	LINPHONE_STATUS_BERIGHTBACK,
	LINPHONE_STATUS_AWAY,
	LINPHONE_STATUS_ONTHEPHONE,
	LINPHONE_STATUS_OUTTOLUNCH,
	LINPHONE_STATUS_NOT_DISTURB,
	LINPHONE_STATUS_MOVED,
	LINPHONE_STATUS_ALT_SERVICE,
	LINPHONE_STATUS_OFFLINE,
	LINPHONE_STATUS_PENDING,
	LINPHONE_STATUS_CLOSED,
	LINPHONE_STATUS_END
}LinphoneOnlineStatus;

typedef enum _LinphoneFirewallPolicy{
	LINPHONE_POLICY_NO_FIREWALL,
	LINPHONE_POLICY_USE_NAT_ADDRESS,
	LINPHONE_POLICY_USE_STUN
} LinphoneFirewallPolicy;

typedef struct _LinphoneCore
{
	LinphoneCoreVTable vtable;
	struct _LpConfig *config;
	net_config_t net_conf;
	sip_config_t sip_conf;
	rtp_config_t rtp_conf;
	sound_config_t sound_conf;
	video_config_t video_conf;
	codecs_config_t codecs_conf;
	ui_config_t ui_conf;
	autoreplier_config_t autoreplier_conf;
	LinphoneProxyConfig *default_proxy;
	MSList *friends;
	MSList *auth_info;
	struct _RingStream *ringstream;
	LCCallbackObj preview_finished_cb;
	bool_t preview_finished;
	LinphoneCall *call;   /* the current call, in the future it will be a list of calls (conferencing)*/
	int rid; /*registration id*/
	MSList *queued_calls;	/* used by the autoreplier */
	MSList *call_logs;
	MSList *chatrooms;
	int max_call_logs;
	int missed_calls;
	struct _AudioStream *audiostream;  /**/
	struct _VideoStream *videostream;
	struct _VideoStream *previewstream;
	struct _RtpProfile *local_profile;
	MSList *subscribers;	/* unknown subscribers */
	int minutes_away;
	LinphoneOnlineStatus presence_mode;
	LinphoneOnlineStatus prev_mode;
	char *alt_contact;
	void *data;
	ms_mutex_t lock;
	char *play_file;
	char *rec_file;
	time_t prevtime;
	int dw_audio_bw;
	int up_audio_bw;
	int dw_video_bw;
	int up_video_bw;
	int automatic_action;
	bool_t use_files;
	bool_t apply_nat_settings;
#ifdef VINCENT_MAURY_RSVP
	/* QoS parameters*/
	int rsvp_enable;
	int rpc_enable;
#endif
} LinphoneCore;

#define LINPHONE_IPADDR_SIZE 64

/* THE main API */

void linphone_core_enable_logs(FILE *file);
void linphone_core_enable_logs_with_cb(OrtpLogFunc logfunc);
void linphone_core_disable_logs(void);

const char *linphone_core_get_version(void);

LinphoneCore *linphone_core_new(const LinphoneCoreVTable *vtable,
						const char *config_path, void* userdata);

void linphone_core_init(LinphoneCore *lc, const LinphoneCoreVTable *vtable,
						const char *config_path, void * userdata);

/* function to be periodically called in a main loop */
void linphone_core_iterate(LinphoneCore *lc);

int linphone_core_invite(LinphoneCore *lc, const char *url);

int linphone_core_refer(LinphoneCore *lc, const char *url);

bool_t linphone_core_inc_invite_pending(LinphoneCore*lc);

int linphone_core_accept_call(LinphoneCore *lc, const char *url);

int linphone_core_terminate_call(LinphoneCore *lc, const char *url);

void linphone_core_send_dtmf(LinphoneCore *lc,char dtmf);

int linphone_core_set_primary_contact(LinphoneCore *lc, const char *contact);

const char *linphone_core_get_primary_contact(LinphoneCore *lc);

void linphone_core_set_guess_hostname(LinphoneCore *lc, bool_t val);
bool_t linphone_core_get_guess_hostname(LinphoneCore *lc);

bool_t linphone_core_ipv6_enabled(LinphoneCore *lc);
void linphone_core_enable_ipv6(LinphoneCore *lc, bool_t val);

osip_from_t *linphone_core_get_primary_contact_parsed(LinphoneCore *lc);

/*0= no bandwidth limit*/
void linphone_core_set_download_bandwidth(LinphoneCore *lc, int bw);
void linphone_core_set_upload_bandwidth(LinphoneCore *lc, int bw);

int linphone_core_get_download_bandwidth(const LinphoneCore *lc);
int linphone_core_get_upload_bandwidth(const LinphoneCore *lc);


#ifdef VINCENT_MAURY_RSVP
/* QoS functions */
int linphone_core_set_rpc_mode(LinphoneCore *lc, int on); /* on = 1 (RPC_ENABLE = 1) */
int linphone_core_set_rsvp_mode(LinphoneCore *lc, int on); /* on = 1 (RSVP_ENABLE = 1) */
int linphone_core_change_qos(LinphoneCore *lc, int answer); /* answer = 1 for yes, 0 for no */
#endif

/* returns a MSList of PayloadType */
const MSList *linphone_core_get_audio_codecs(const LinphoneCore *lc);

int linphone_core_set_audio_codecs(LinphoneCore *lc, MSList *codecs);
/* returns a MSList of PayloadType */
const MSList *linphone_core_get_video_codecs(const LinphoneCore *lc);

int linphone_core_set_video_codecs(LinphoneCore *lc, MSList *codecs);

bool_t linphone_core_check_payload_type_usability(const LinphoneCore *lc, PayloadType *pt);

void linphone_core_add_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *config);

void linphone_core_remove_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *config);

const MSList *linphone_core_get_proxy_config_list(const LinphoneCore *lc);

void linphone_core_set_default_proxy(LinphoneCore *lc, LinphoneProxyConfig *config);

void linphone_core_set_default_proxy_index(LinphoneCore *lc, int index);

int linphone_core_get_default_proxy(LinphoneCore *lc, LinphoneProxyConfig **config);

void linphone_core_add_auth_info(LinphoneCore *lc, LinphoneAuthInfo *info);

void linphone_core_remove_auth_info(LinphoneCore *lc, LinphoneAuthInfo *info);

void linphone_core_clear_all_auth_info(LinphoneCore *lc);

int linphone_core_get_audio_jittcomp(LinphoneCore *lc);

void linphone_core_set_audio_jittcomp(LinphoneCore *lc, int value);

int linphone_core_get_audio_port(const LinphoneCore *lc);

int linphone_core_get_video_port(const LinphoneCore *lc);

void linphone_core_set_audio_port(LinphoneCore *lc, int port);

void linphone_core_set_video_port(LinphoneCore *lc, int port);

void linphone_core_set_use_info_for_dtmf(LinphoneCore *lc, bool_t use_info);

bool_t linphone_core_get_use_info_for_dtmf(LinphoneCore *lc);

int linphone_core_get_sip_port(LinphoneCore *lc);

void linphone_core_set_sip_port(LinphoneCore *lc,int port);

void linphone_core_set_inc_timeout(LinphoneCore *lc, int seconds);

int linphone_core_get_inc_timeout(LinphoneCore *lc);

void linphone_core_set_stun_server(LinphoneCore *lc, const char *server);

const char * linphone_core_get_stun_server(const LinphoneCore *lc);

void linphone_core_set_nat_address(LinphoneCore *lc, const char *addr);

const char *linphone_core_get_nat_address(const LinphoneCore *lc);

void linphone_core_set_firewall_policy(LinphoneCore *lc, LinphoneFirewallPolicy pol);

LinphoneFirewallPolicy linphone_core_get_firewall_policy(const LinphoneCore *lc);

/* sound functions */
/* returns a null terminated static array of string describing the sound devices */ 
const char**  linphone_core_get_sound_devices(LinphoneCore *lc);
int linphone_core_get_ring_level(LinphoneCore *lc);
int linphone_core_get_play_level(LinphoneCore *lc);
int linphone_core_get_rec_level(LinphoneCore *lc);
void linphone_core_set_ring_level(LinphoneCore *lc, int level);
void linphone_core_set_play_level(LinphoneCore *lc, int level);
void linphone_core_set_rec_level(LinphoneCore *lc, int level);
const char * linphone_core_get_ringer_device(LinphoneCore *lc);
const char * linphone_core_get_playback_device(LinphoneCore *lc);
const char * linphone_core_get_capture_device(LinphoneCore *lc);
int linphone_core_set_ringer_device(LinphoneCore *lc, const char * devid);
int linphone_core_set_playback_device(LinphoneCore *lc, const char * devid);
int linphone_core_set_capture_device(LinphoneCore *lc, const char * devid);
char linphone_core_get_sound_source(LinphoneCore *lc);
void linphone_core_set_sound_source(LinphoneCore *lc, char source);
void linphone_core_set_ring(LinphoneCore *lc, const char *path);
char *linphone_core_get_ring(LinphoneCore *lc);
int linphone_core_preview_ring(LinphoneCore *lc, const char *ring,LinphoneCoreCbFunc func,void * userdata);
typedef enum {RINGBACK_TYPE_FR,RINGBACK_TYPE_US} RingBackType;
void linphone_core_set_ringback(LinphoneCore *lc,RingBackType type);
RingBackType linphone_core_get_ringback(LinphoneCore *lc);
void linphone_core_enable_echo_cancelation(LinphoneCore *lc, bool_t val);
bool_t linphone_core_echo_cancelation_enabled(LinphoneCore *lc);

void linphone_core_set_presence_info(LinphoneCore *lc,int minutes_away,const char *contact,LinphoneOnlineStatus os);

void linphone_core_add_friend(LinphoneCore *lc, LinphoneFriend *fr);
void linphone_core_remove_friend(LinphoneCore *lc, LinphoneFriend *fr);
void linphone_core_reject_subscriber(LinphoneCore *lc, LinphoneFriend *lf);
/* a list of LinphoneFriend */
MSList * linphone_core_get_friend_list(LinphoneCore *lc);
/* notify all friends that have subscribed */
void linphone_core_notify_all_friends(LinphoneCore *lc, LinphoneOnlineStatus os);

/* returns a list of LinphoneCallLog */
MSList * linphone_core_get_call_logs(LinphoneCore *lc);

/* video support */
void linphone_core_enable_video(LinphoneCore *lc, bool_t vcap_enabled, bool_t display_enabled);
bool_t linphone_core_video_enabled(LinphoneCore *lc);

/*play/record support: use files instead of soundcard*/
void linphone_core_use_files(LinphoneCore *lc, bool_t yesno);
void linphone_core_set_play_file(LinphoneCore *lc, const char *file);
void linphone_core_set_record_file(LinphoneCore *lc, const char *file);


/*
void linphone_core_enable_video_preview(LinphoneCore *lc, bool_t val);
bool_t linphone_core_video_preview_enabled(LinphoneCore *lc);
*/

int linphone_core_set_video_device(LinphoneCore *lc, const char *method, const char *device);

bool_t linphone_core_is_in_main_thread(LinphoneCore *lc);

void *linphone_core_get_user_data(LinphoneCore *lc);

/* returns LpConfig object to read/write to the config file: usefull if you wish to extend
the config file with your own sections */
struct _LpConfig *linphone_core_get_config(LinphoneCore *lc);

void linphone_core_uninit(LinphoneCore *lc);
void linphone_core_destroy(LinphoneCore *lc);

/* end of lecacy api */

/*internal use only */
#define linphone_core_lock(lc)	ms_mutex_lock(&(lc)->lock)
#define linphone_core_unlock(lc)	ms_mutex_unlock((&lc)->lock)
void linphone_core_start_media_streams(LinphoneCore *lc, LinphoneCall *call);
void linphone_core_stop_media_streams(LinphoneCore *lc);
const char * linphone_core_get_identity(LinphoneCore *lc);
const char * linphone_core_get_route(LinphoneCore *lc);
bool_t linphone_core_interpret_url(LinphoneCore *lc, const char *url, char **real_url, osip_to_t **real_parsed_url, char **route);

#ifdef __cplusplus
}
#endif

#endif
