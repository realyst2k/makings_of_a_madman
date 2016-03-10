// Written by Réal 'realyst' O'Neil
// for http://www.cartridgeclub.org/makings-of-a-madman
// March, 2016
//
// Feel free to use in your own weeblies, I'd just appreciate
// you refer back to my site.
//
// Hack in a wiki style TOC into Weebly's limited feature set.
// create a Title box in Weebly with only '[[TOC]]' in it
// Add a leading '_' to titles you want to indent.
// more '_' means more indentation
// Add this code to the header code section of the
// "Blog Settings" panel in the blog "build" editor,
// not the page editor bar.

// It's advised you put the [[TOC]] title behind the 
// "Read More" section of a blog post.

// Alternatively, alter the jQuery 'each' statement
// to target a different class and create a hidden
// div using the "Embed Code" object
// but that's far more steps.

/* for Mozilla/Opera9 */
if (document.addEventListener) {
  document.addEventListener("DOMContentLoaded", toc_gen, false);
}

/* for Internet Explorer */
/*@cc_on @*/
/*@if (@_win32)
  document.write("<script id=__ie_onload defer src=javascript:void(0)><\/script>");
  var script = document.getElementById("__ie_onload");
  script.onreadystatechange = function() {
    if (this.readyState == "complete") {
      toc_gen(); // call the onload handler
    }
  };
/*@end @*/

/* for Safari */
if (/WebKit/i.test(navigator.userAgent)) { // sniff
  var _timer = setInterval(function() {
    if (/loaded|complete/.test(document.readyState)) {
      toc_gen(); // call the onload handler
    }
  }, 10);
}

/* for other browsers */
window.onload = toc_gen;

function toc_gen () { 
     // quit if this function has already been called
  if (arguments.callee.done) return;

  // flag this function so we don't do the same thing twice
  arguments.callee.done = true;

  // kill the timer
  if (_timer) clearInterval(_timer);
  (function($){ 
    $('.wsite-content-title').each(
        function(key,val){
            if($(val).text().match("[[TOC]]")) {
                var lst=document.createElement('ul');
                lst.setAttribute('class','toc-list');
                var tocdiv=document.createElement('div');
                tocdiv.setAttribute('id','toc-container');
                var toctitlespan=document.createElement('span');
                toctitlespan.style.color='white';
                toctitlespan.style.textDecoration='underline';
                toctitlespan.style.fontWeight='bold';
                toctitlespan.style.width='100%';
                toctitlespan.appendChild(document.createTextNode('Table of Contents'));
                tocdiv.appendChild(toctitlespan);
                tocdiv.appendChild(lst);
                $(val).empty();
                $(val).replaceWith(tocdiv);
            }
            else {
                console.log($(val).text());
                var a=document.createElement('a');
                var atitle=$(val).text().replace(/[^a-zA-Z0-9]/g,'');
                a.setAttribute('name',atitle);
                val.appendChild(a);
                var lk=document.createElement('a');
                if($(val).text().match(/^(_+)/)) {
                    var us_matches=$(val).text().match(/^(_+)+/);
                    console.log(us_matches[0]);
                    var t=$(val).text().replace(/^_+/,'');
                    $(val).text(t);
                    $(val).addClass('toc-subtitle-flag');
                    lk.style.paddingLeft=(us_matches[0].length*20)+'px';
                }
                lk.appendChild(document.createTextNode($(val).text()));
                lk.setAttribute('href','#'+atitle);
                var lie=document.createElement('li');
                lie.setAttribute('class','toc-item');
                lie.appendChild(lk);
                $('.toc-list').each(function(nm,el){ el.appendChild(lie)})              
            }
        })

})(jQuery)
}


