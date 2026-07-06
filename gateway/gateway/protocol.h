#ifndef PROTOCOL_H
#define PROTOCOL_H

// 消息类型定义
#define MSG_GET_COURSE "get_course"
#define MSG_SEND_MESSAGE "send_message"
#define MSG_GET_SEAT_STATUS "get_seat_status"
#define MSG_RESERVE_SEAT "reserve_seat"
#define MSG_CANCEL_RESERVATION "cancel_reservation"
#define MSG_GET_NOTICES "get_notices"
#define MSG_GET_NOTICE_DETAIL "get_notice_detail"
#define MSG_GET_ATTENDANCE_LIST "get_attendance_list"
#define MSG_SUBMIT_ATTENDANCE "submit_attendance"

// 响应类型
#define RESP_COURSE_INFO "course_info"
#define RESP_MESSAGE_RESULT "message_response"
#define RESP_SEAT_STATUS "seat_status"
#define RESP_RESERVE_RESULT "reserve_response"
#define RESP_NOTICE_LIST "notice_list"
#define RESP_NOTICE_DETAIL "notice_detail"
#define RESP_ATTENDANCE_LIST "attendance_list"
#define RESP_ATTENDANCE_RESULT "attendance_response"

#define MAX_BUFFER_SIZE 16384

#endif
