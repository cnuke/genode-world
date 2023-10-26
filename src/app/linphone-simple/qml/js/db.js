/*
 * uNav http://launchpad.net/unav
 * Copyright (C) 2015 JkB https://launchpad.net/~joergberroth
 * Copyright (C) 2015 Marcos Alvarez Costales https://launchpad.net/~costales
 *
 * uNav is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * uNav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
// Thanks http://askubuntu.com/questions/352157/how-to-use-a-sqlite-database-from-qml


function openDB() {

    var db = LocalStorage.openDatabaseSync("favcontacts_db", "0.1", "Favorite Contacts", 1000);
    try {
        db.transaction(function(tx){
            tx.executeSql('CREATE TABLE IF NOT EXISTS latestcontacts( revision DATE, identifier INTEGER PRIMARY KEY, name TEXT, sipaddress TEXT, icon TEXT )');
            tx.executeSql('CREATE TABLE IF NOT EXISTS    favcontacts( revision DATE, identifier INTEGER PRIMARY KEY, name TEXT, sipaddress TEXT, icon TEXT )');
        });
    } catch (err) {
        console.log("Error creating table in database: " + err)
    } return db
}

// Save Contact Info
function storeContact(revision, name, sipaddress, icon) {
    var db = openDB();

    var already_stored = false;

    /* assume the sipaddress always is unique */
    db.transaction(function(tx) {
        var rs = tx.executeSql('SELECT * FROM latestcontacts WHERE sipaddress=?;', [sipaddress]);

        if (rs.rows.length > 0) {
            already_stored = true;
        }
    });

    if (already_stored)
        return;

    db.transaction(function(tx){
        tx.executeSql('INSERT OR REPLACE INTO latestcontacts (revision, name, sipaddress, icon) VALUES(?, ?, ?, ?)', [revision, name, sipaddress, icon]); //null is the automatic id
    });
}

function storeFavContact(revision, id, name, sipaddress, icon) {
    var db = openDB();
    db.transaction(function(tx){
        tx.executeSql('INSERT OR REPLACE INTO favcontacts (revision, identifier, name, sipaddress, icon) VALUES(?, ?, ?, ?, ?)', [revision, id, name, sipaddress, icon]); //null is the automatic id
    });
}

function getNumberOfContacts() {
    var db = openDB();
    var rs;
    db.transaction(function(tx) {
        rs = tx.executeSql('SELECT * FROM latestcontacts');
    });

    return rs.rows.lenght
}

function getLatestContacts() {
    var db = openDB();
    var rs;
    db.transaction(function(tx) {
        rs = tx.executeSql('SELECT * FROM latestcontacts');
    });

    return rs
}

function getFavorites() {
    var db = openDB();
    var rs;
    db.transaction(function(tx) {
        rs = tx.executeSql('SELECT * FROM favcontacts');
    });

    return rs
}

function getFavorite(id) {
    var fav_name = "";
    var fav_sipaddress = "";
    var db = openDB();

    db.transaction(function(tx) {
        var rs = tx.executeSql('SELECT name, sipaddress FROM favcontacts WHERE identifier=?;', [id]);

        if (rs.rows.length > 0) {
            fav_name = rs.rows.item(0).name;
            fav_sipaddress = rs.rows.item(0).sipaddress;
        } else {
            fav_name = null;
            fav_sipaddress = null;
            console.log("getFavorite: info set to null");
        }

    });

    return [fav_name, fav_sipaddress];
}

function deleteContact(ident){
    var db = openDB();
    db.transaction( function(tx){
        tx.executeSql('DELETE FROM latestcontacts WHERE identifier=?;', [ident]);
    });
}

function deleteFavorite(ident){
    var db = openDB();
    db.transaction( function(tx){
        tx.executeSql('DELETE FROM favcontacts WHERE identifier=?;', [ident]);
    });
}
