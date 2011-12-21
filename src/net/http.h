/***************************************************************************
    begin........: October 2007
    copyright....: Sebastian Fedrau
    email........: lord-kefir@arcor.de
 ***************************************************************************/

/***************************************************************************
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.
 ***************************************************************************/
/*!
 * \file http.h
 * \brief HTTP status codes.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 11. October 2007
 */

#ifndef _HTTP_H
#define _HTTP_H

/**
 * @addtogroup Net
 * @{
 * 	@addtogroup Http
 * 	@{
 */

/*! Default HTTP port. */
#define HTTP_DEFAULT_PORT                      80
/*! Default HTTP SSL port. */
#define HTTP_DEFAULT_SSL_PORT                  443

/*! Undefined HTTP status. */
#define HTTP_NONE                              -1

/*! HTTP status code 100. */
#define HTTP_CONTINUE                          100
/*! HTTP status code 101. */
#define HTTP_SWITCHING_PROTOCOLS               101
/*! HTTP status code 102. */
#define HTTP_PROCESSING                        102

/*! HTTP status code 200. */
#define HTTP_OK                                200
/*! HTTP status code 201. */
#define HTTP_CREATED                           201
/*! HTTP status code 202. */
#define HTTP_ACCEPTED                          202
/*! HTTP status code 203. */
#define HTTP_NON_AUTHORITATIVE_INFORMATION     203
/*! HTTP status code 204. */
#define HTTP_NO_CONTENT                        204
/*! HTTP status code 205. */
#define HTTP_RESET_CONTENT                     205
/*! HTTP status code 206. */
#define HTTP_PARTIAL_CONTENT                   206
/*! HTTP status code 207. */
#define HTTP_MULTI_STATUS                      207

/*! HTTP status code 300. */
#define HTTP_MULTIPLE_CHOICES                  300
/*! HTTP status code 301. */
#define HTTP_MOVED_PERMANENTLY                 301
/*! HTTP status code 302. */
#define HTTP_FOUND                             302
/*! HTTP status code 303. */
#define HTTP_SEE_OTHER                         303
/*! HTTP status code 304. */
#define HTTP_NOT_MODIFIED                      304
/*! HTTP status code 305. */
#define HTTP_USE_PROXY                         305
/*! HTTP status code 306. */
#define HTTP_SWITCH_PROXY                      306
/*! HTTP status code 307. */
#define HTTP_TEMPORARY_REDIRECT                307

/*! HTTP status code 400. */
#define HTTP_BAD_REQUEST                       400
/*! HTTP status code 401. */
#define HTTP_UNAUTHORIZED                      401
/*! HTTP status code 402. */
#define HTTP_PAYMENT_REQUIRED                  402
/*! HTTP status code 403. */
#define HTTP_FORBIDDEN                         403
/*! HTTP status code 404. */
#define HTTP_NOT_FOUND                         404
/*! HTTP status code 405. */
#define HTTP_METHOD_NOT_ALLOWED                405
/*! HTTP status code 406. */
#define HTTP_NOT_ACCEPTABLE                    406
/*! HTTP status code 407. */
#define HTTP_PROXY_AUTHENTICATION_REQUIRED     407
/*! HTTP status code 408. */
#define HTTP_REQUEST_TIMEOUT                   408
/*! HTTP status code 409. */
#define HTTP_CONFLICT                          409
/*! HTTP status code 410. */
#define HTTP_GONE                              410
/*! HTTP status code 411. */
#define HTTP_LENGTH_REQUIRED                   411
/*! HTTP status code 412. */
#define HTTP_PRECONDITION_FAILED               412
/*! HTTP status code 413. */
#define HTTP_REQUEST_ENTITIY_TOO_LONG          413
/*! HTTP status code 414. */
#define HTTP_REQUEST_URI_TOO_LONG              414
/*! HTTP status code 415. */
#define HTTP_UNSUPPORTED_MEDIA_TYPE            415
/*! HTTP status code 416. */
#define HTTP_REQUESTED_RANGE_NOT_SATISFIABLE   416
/*! HTTP status code 417. */
#define HTTP_EXPECTATION_FAILED                417
/*! HTTP status code 422. */
#define HTTP_UNPROCESSABLE_ENTITIY             422
/*! HTTP status code 423. */
#define HTTP_LOCKED                            423
/*! HTTP status code 424. */
#define HTTP_FAILED_DEPENDENCY                 424
/*! HTTP status code 425. */
#define HTTP_UNORDERED_COLLECTION              425
/*! HTTP status code 426. */
#define HTTP_UPGRADE_REQUIRED                  426
/*! HTTP status code 449. */
#define HTTP_RETRY_WITH                        449

/*! HTTP status code 500. */
#define HTTP_INTERNAL_SERVER_ERROR             500
/*! HTTP status code 501. */
#define HTTP_NOT_IMPLEMENTED                   501
/*! HTTP status code 502. */
#define HTTP_BAD_GATEWAY                       502
/*! HTTP status code 503. */
#define HTTP_SERVICE_TEMPORARILY_UNAVAILABLE   503
/*! HTTP status code 504. */
#define HTTP_GATEWAY_TIMEOUT                   504
/*! HTTP status code 505. */
#define HTTP_VERSION_NOT_SUPPORTED             505
/*! HTTP status code 506. */
#define HTTP_VARIANT_ALSO_NEGOTIATES           506
/*! HTTP status code 507. */
#define HTTP_INSUFFICIENT_STORAGE              507
/*! HTTP status code 509. */
#define HTTP_BANDWIDTH_LIMIT_EXCEEDED          509
/*! HTTP status code 510. */
#define HTTP_NOT_EXTENDED                      510

/**
 * @}
 * @}
 */
#endif

