/***************************************************************************
    begin........: November 2010
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
 * \file twitterdb_queries.c
 * \brief SQL Query collection.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 2. March 2012
 */

#include "twitterdb_queries.h"

/**
 * @addtogroup Core
 * @{
 * 	@addtogroup Twitter
 * 	@{
 */

const gchar *twitterdb_queries_create_tables[] =
{
	"BEGIN TRANSACTION",

	"CREATE TABLE IF NOT EXISTS user ("
	"guid VARCHAR(32) NOT NULL, "
	"username VARCHAR(64) NOT NULL, "
	"realname VARCHAR(64) NOT NULL, "
	"image VARCHAR(64) NOT NULL, "
	"location VARCHAR(64) NOT NULL, "
	"website VARCHAR(256) NOT NULL, "
	"description VARCHAR(256) NOT NULL, "
	"PRIMARY KEY(guid))",

	"CREATE TABLE IF NOT EXISTS status ("
	"guid VARCHAR(32) NOT NULL, "
	"prev_status VARCHAR(32), "
	"text VARCHAR(140) NOT NULL, "
	"user_guid VARCHAR(32) NOT NULL REFERENCES user(guid) ON DELETE CASCADE, "
	"timestamp INTEGER NOT NULL, "
	"read BOOLEAN NOT NULL, "
	"PRIMARY KEY(guid))",

	"CREATE TABLE IF NOT EXISTS follower ("
	"user1_guid VARCHAR(32) NOT NULL REFERENCES user(guid) ON DELETE CASCADE, "
	"user2_guid VARCHAR(32) NOT NULL REFERENCES user(guid) ON DELETE CASCADE, "
	"PRIMARY KEY(user1_guid, user2_guid))",
	
	"CREATE TABLE IF NOT EXISTS list("
	"guid VARCHAR(16) NOT NULL, "
	"user_guid VARCHAR(32) NOT NULL REFERENCES user(guid) ON DELETE CASCADE, "
	"name VARCHAR(64) NOT NULL, "
	"fullname VARCHAR(64) NOT NULL, "
	"uri VARCHAR(256) NOT NULL, "
	"description VARCHAR(256) NOT NULL, "
	"protected BOOLEAN NOT NULL, "
	"subscriber_count INTEGER NOT NULL, "
	"member_count INTEGER NOT NULL, "
	"PRIMARY KEY(guid))",

	"CREATE TABLE IF NOT EXISTS list_timeline ("
	"list_guid VARCHAR(32) NOT NULL REFERENCES list(guid) ON DELETE CASCADE, "
	"status_guid VARCHAR(32) NOT NULL REFERENCES status(guid) ON DELETE CASCADE, "
	"PRIMARY KEY(list_guid, status_guid))",

	"CREATE TABLE IF NOT EXISTS list_member ("
	"list_guid VARCHAR(32) NOT NULL REFERENCES list(guid) ON DELETE CASCADE, "
	"user_guid VARCHAR(32) NOT NULL REFERENCES user(guid) ON DELETE CASCADE, "
	"PRIMARY KEY(list_guid, user_guid))",

	"CREATE TABLE IF NOT EXISTS timeline ("
	"user_guid VARCHAR(32) NOT NULL REFERENCES user(guid) ON DELETE CASCADE, "
	"status_guid VARCHAR(32) NOT NULL REFERENCES status(guid) ON DELETE CASCADE, "
	"type INTEGER NOT NULL, "
	"PRIMARY KEY(user_guid, status_guid, type))",

 	"CREATE TABLE IF NOT EXISTS direct_message("
	"guid VARCHAR(32) NOT NULL, "
	"text VARCHAR(140) NOT NULL, "
	"timestamp INTEGER NOT NULL, "
	"sender_guid VARCHAR(32) NOT NULL REFERENCES user(guid) ON DELETE CASCADE, "
	"receiver_guid VARCHAR(32) NOT NULL REFERENCES user(guid) ON DELETE CASCADE, "
	"PRIMARY KEY(guid))",

 	"CREATE TABLE IF NOT EXISTS last_sync("
	"source INTEGER NOT NULL, "
	"user_guid VARCHAR(32) NOT NULL, "
	"seconds INTEGER NOT NULL, "
	"PRIMARY KEY(source, user_guid))",

	"CREATE TABLE IF NOT EXISTS version ("
	"id INTEGER NOT NULL, "
	"major INTEGER NOT NULL, "
	"minor INTEGER NOT NULL, "
	"PRIMARY KEY(id))",

	"COMMIT",

	NULL
};

const gchar *twitterdb_queries_replace_version = "REPLACE INTO version (id, major, minor) VALUES (1, ?, ?)";

const gchar *twitterdb_queries_count_table = "SELECT COUNT(name) FROM sqlite_master WHERE name=? COLLATE NOCASE";

const gchar *twitterdb_queries_get_version = "SELECT major, minor FROM version";

const gchar *twitterdb_queries_get_user = "SELECT username, realname, image, location, website, description FROM user WHERE guid=?";

const gchar *twitterdb_queries_map_username = "SELECT guid FROM user WHERE username=? COLLATE NOCASE";

const gchar *twitterdb_queries_user_exists = "SELECT COUNT(guid) FROM user WHERE guid=?";

const gchar *twitterdb_queries_insert_user = "INSERT INTO user (guid, username, realname, image, location, website, description) VALUES (?, ?, ?, ?, ?, ?, ?)";

const gchar *twitterdb_queries_update_user = "UPDATE user SET username=?, realname=?, image=?, location=?, website=?, description=? WHERE guid=?";

const gchar *twitterdb_queries_mark_statuses_read = "UPDATE status SET read=1";

const gchar *twitterdb_queries_status_exists = "SELECT COUNT(guid) FROM status WHERE guid=?";

const gchar *twitterdb_queries_insert_status = "INSERT INTO status (guid, prev_status, text, user_guid, timestamp, read) VALUES (?, ?, ?, ?, ?, 0)";

const gchar *twitterdb_queries_delete_status = "DELETE FROM status WHERE guid=?";

const gchar *twitterdb_queries_get_status = "SELECT text, user_guid, timestamp, prev_status FROM status WHERE guid=?";

const gchar *twitterdb_queries_replace_follower = "REPLACE INTO follower (user1_guid, user2_guid) VALUES (?, ?)";

const gchar *twitterdb_queries_delete_follower = "DELETE FROM follower WHERE user1_guid=? AND user2_guid=?";

const gchar *twitterdb_queries_remove_friends_from_user = "DELETE FROM follower WHERE user1_guid=?";

const gchar *twitterdb_queries_get_friends = "SELECT user2_guid FROM follower WHERE user1_guid=?";

const gchar *twitterdb_queries_get_followers = "SELECT user1_guid FROM follower WHERE user2_guid=?";

const gchar *twitterdb_queries_remove_followers_from_user = "DELETE FROM follower WHERE user2_guid=?";

const gchar *twitterdb_queries_is_follower = "SELECT COUNT(user1_guid) FROM follower "
                                             "INNER JOIN \"user\" AS user1 ON follower.user1_guid=user1.guid "
                                             "INNER JOIN \"user\" AS user2 ON follower.user2_guid=user2.guid WHERE user1.username=? COLLATE NOCASE AND user2.username=? COLLATE NOCASE";

const gchar *twitterdb_queries_count_tweets_from_list = "SELECT COUNT(list_guid) FROM list_timeline WHERE list_guid=?";

const gchar *twitterdb_queries_get_list = "SELECT guid, name, fullname, uri, description, protected, subscriber_count, member_count FROM list WHERE guid=?";

const gchar *twitterdb_queries_get_lists_following_user = "SELECT DISTINCT(list_guid) FROM list_member WHERE user_guid=?";

const gchar *twitterdb_queries_insert_list = "INSERT INTO list (guid, user_guid, name, fullname, protected, uri, description, subscriber_count, member_count) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

const gchar *twitterdb_queries_update_list = "UPDATE list SET user_guid=?, name=?, fullname=?, protected=?, uri=?, description=?, subscriber_count=?, member_count=? WHERE guid=?";

const gchar *twitterdb_queries_list_exists = "SELECT COUNT(guid) FROM list WHERE guid=?";

const gchar *twitterdb_queries_delete_list = "DELETE FROM list WHERE guid=?";

const gchar *twitterdb_queries_get_list_guid = "SELECT list.guid FROM list INNER JOIN \"user\" ON list.user_guid=\"user\".guid WHERE list.name=? COLLATE NOCASE AND \"user\".username=? COLLATE NOCASE";

const gchar *twitterdb_queries_get_list_guids_from_user = "SELECT guid FROM list WHERE user_guid=? ORDER BY name";

const gchar *twitterdb_queries_remove_obsolete_statuses_from_list =
	"DELETE FROM list_timeline WHERE list_timeline.list_guid=? AND list_timeline.status_guid IN "
	"(SELECT status.guid FROM list_timeline INNER JOIN status ON status.guid=list_timeline.status_guid WHERE list_guid=? AND status.user_guid NOT IN "
	"(SELECT list_member.user_guid FROM list_member WHERE list_member.list_guid=?))";

const gchar *twitterdb_queries_replace_status_into_timeline = "REPLACE INTO timeline (user_guid, status_guid, type) VALUES (?, ?, ?)";

const gchar *twitterdb_queries_replace_into_list_timeline = "REPLACE INTO list_timeline (list_guid, status_guid) VALUES (?, ?)";

const gchar *twitterdb_queries_delete_list_members = "DELETE FROM list_member WHERE list_guid=?";

const gchar *twitterdb_queries_replace_into_list_member = "REPLACE INTO list_member (list_guid, user_guid) VALUES (?, ?)";

const gchar *twitterdb_queries_get_list_members =
	"SELECT member.guid, member.username, member.realname, member.image, member.location, member.website, member.description FROM list_member "
	"INNER JOIN list ON list.guid=list_member.list_guid "
	"INNER JOIN \"user\" AS member ON member.guid=list_member.user_guid "
	"INNER JOIN \"user\" AS owner ON owner.guid=list.user_guid "
	"WHERE owner.username=? COLLATE NOCASE AND list.name=? COLLATE NOCASE ORDER BY member.username";

const gchar *twitterdb_queries_delete_user_from_list = "DELETE FROM list_member WHERE list_guid=? AND user_guid=?";

const gchar *twitterdb_queries_direct_message_exists = "SELECT COUNT(guid) FROM direct_message WHERE guid=?";

const gchar *twitterdb_queries_insert_direct_message = "INSERT INTO direct_message (guid, text, timestamp, sender_guid, receiver_guid) VALUES (?, ?, ?, ?, ?)";

const gchar *twitterdb_queries_get_tweets_from_timeline =
	"SELECT status_guid, text, \"timestamp\", read, status.user_guid, "
	"publisher.username, publisher.realname, publisher.image, publisher.location, publisher.website, publisher.description, status.prev_status "
	"FROM timeline "
	"INNER JOIN status ON status.guid=timeline.status_guid "
	"INNER JOIN \"user\" AS owner ON owner.guid=timeline.user_guid "
	"INNER JOIN \"user\" AS publisher ON publisher.guid=status.user_guid "
	"WHERE type=? AND owner.username=? COLLATE NOCASE "
	"ORDER BY \"timestamp\" DESC LIMIT 100";

const gchar *twitterdb_queries_get_new_tweets_from_timeline =
	"SELECT status_guid, text, \"timestamp\", read, status.user_guid, "
	"publisher.username, publisher.realname, publisher.image, publisher.location, publisher.website, publisher.description, status.prev_status "
	"FROM timeline "
	"INNER JOIN status ON status.guid=timeline.status_guid "
	"INNER JOIN \"user\" AS owner ON owner.guid=timeline.user_guid "
	"INNER JOIN \"user\" AS publisher ON publisher.guid=status.user_guid "
	"WHERE type=? AND owner.username=? COLLATE NOCASE AND read=0 "
	"ORDER BY \"timestamp\" DESC LIMIT 100";

const gchar *twitterdb_queries_get_tweets_from_list =
	"SELECT status_guid, text, \"timestamp\", read, status.user_guid, "
	"publisher.username, publisher.realname, publisher.image, publisher.location, publisher.website, publisher.description, status.prev_status "
	"FROM list_timeline "
	"INNER JOIN status ON status.guid=list_timeline.status_guid "
	"INNER JOIN list ON list_timeline.list_guid=list.guid "
	"INNER JOIN \"user\" AS publisher ON publisher.guid=status.user_guid "
	"INNER JOIN \"user\" AS owner ON owner.guid=list.user_guid "
	"WHERE owner.username=? COLLATE NOCASE AND list.name=? COLLATE NOCASE "
	"ORDER BY \"timestamp\" DESC LIMIT 100";

const gchar *twitterdb_queries_get_list_membership =
	"SELECT owner.username, list.name FROM list "
	"INNER JOIN \"user\" AS owner ON owner.guid=list.user_guid "
	"INNER JOIN list_member ON list_member.list_guid=list.guid "
	"INNER JOIN \"user\" ON \"user\".guid=list_member.user_guid "
	"WHERE \"user\".username=? COLLATE NOCASE "
	"ORDER BY list.name";

const gchar *twitterdb_queries_user_is_list_member =
	"SELECT COUNT(list.guid) FROM list "
	"INNER JOIN list_member ON list.guid=list_member.list_guid "
	"INNER JOIN \"user\" AS member ON member.guid=list_member.user_guid "
	"INNER JOIN \"user\" AS owner ON owner.guid=list.user_guid "
	"WHERE member.username=? COLLATE NOCASE AND list.name=? COLLATE NOCASE AND owner.username=? COLLATE NOCASE";

const gchar *twitterdb_queries_get_sync_seconds = "SELECT seconds FROM last_sync WHERE source=? AND user_guid=?";

const gchar *twitterdb_queries_replace_sync_seconds = "REPLACE INTO last_sync (source, user_guid, seconds) VALUES (?, ?, ?)";

const gchar *twitterdb_queries_remove_sync_seconds = "DELETE FROM last_sync WHERE source=?";

const gchar *twitterdb_queries_add_prev_status_column = "ALTER TABLE status ADD COLUMN prev_status VARCHAR(32)";

/**
 * @}
 * @}
 */

