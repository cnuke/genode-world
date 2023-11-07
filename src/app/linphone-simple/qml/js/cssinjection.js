// ==UserScript==
// @name          ifarchive
// @namespace     https://ifarchive.info
// @description
// @author
// @homepage      https://ifarchive.info
// @run-at        document-start
// ==/UserScript==
(function() {var css = [
"aside, footer, #showLeftPush { display: none  !important; }",
"body { background: #ffffff !important; }",
"a, a:hover, h1, h3 { color: #eb6536 !important; }",
".frame input { border-radius: 7px; }",
"#conteneur #content { margin: 0 !important; padding:0 !important; }"
].join("\n");
if (typeof GM_addStyle != "undefined") {
    GM_addStyle(css);
} else if (typeof PRO_addStyle != "undefined") {
    PRO_addStyle(css);
} else if (typeof addStyle != "undefined") {
    addStyle(css);
} else {
    var node = document.createElement("style");
    node.type = "text/css";
    node.appendChild(document.createTextNode(css));
    var heads = document.getElementsByTagName("head");
    if (heads.length > 0) {
        heads[0].appendChild(node);
    } else {
        // no head yet, stick it whereever
        document.documentElement.appendChild(node);
    }
}
})();
