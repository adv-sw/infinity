# infinity
&lt;>&lt;> infinity 3d web tech public interfaces &amp; examples.


Building infinity mozilla/gecko plugin (modified firefox).


1. Clone version of mozilla we currently target.

Version : https://hg.mozilla.org/mozilla-unified/rev/FIREFOX_91_3_0esr_RELEASE

Clone : hg clone https://hg.mozilla.org/mozilla-unified -r 8802d620459d50ea648821a15497d06efd15732b mozilla

2. Clone this repo & place infinity at same level in your dev hierarchy as the mozilla root delivered by above.

3. Add INF_PATCH define to your mozconfig.

# The following to your mozconfig
export CFLAGS="-DINF_PATCH=1"
export CXXFLAGS="-DINF_PATCH=1"

4. Build as normal.

5. Copy mozilla dist/bin directory to [infinity]/programs/mozilla

            e.g. C:\Program Files\Infinity\programs\mozilla

6. Run infinity with test content to test app & any modifications.
