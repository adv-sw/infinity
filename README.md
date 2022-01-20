# infinity
&lt;>&lt;> infinity 3d web tech public interfaces &amp; examples.


Building mozilla/gecko <><> infinity app (modified firefox).


1. Clone version of mozilla targeted by this patch :

            Version : https://hg.mozilla.org/mozilla-unified/rev/FIREFOX_91_4_0esr_RELEASE

            Clone : hg clone https://hg.mozilla.org/mozilla-unified -r 3359a85a2277bbcbcc2b5ebcc425922a23d07f34 mozilla


2. Clone this repo & place infinity at same level in your dev hierarchy as the mozilla root delivered by above.


3. Add INF_PATCH definse to your mozconfig :

            export CFLAGS="-DINF_PATCH=1"
            export CXXFLAGS="-DINF_PATCH=1"


4. Build as normal.

5. Copy built mozilla dist/bin directory over [infinity]/programs/mozilla

            e.g. C:\Program Files\Infinity\programs\mozilla

6. Run infinity with webpage content to test <><> infinity mozilla app & any modifications.



