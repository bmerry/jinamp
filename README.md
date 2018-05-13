Jinamp Is Not An MP3 Player
---------------------------

Q: What is Jinamp?

A: Jinamp is what I use to play my music. I found that MP3 players seemed
to come in two types: players that focused on getting the playing done
and had very limited shuffling and selection ability (like mpg123), and
highly interactive players (like xmms). I wanted a player which would
allow me to specify what I wanted to play on the command line in a
fairly powerful way, then go into the background and not get in my way.
Jinamp is the result.

Q: So why is Jinamp not an MP3 Player?

A: It doesn't contain any decoding code. Instead it invokes an external
player such as mpg123 to do all the hard work. In fact there is no
reason why it can't be used to play other formats such as Ogg Vorbis or
even non-audio data.

Q: Does Jinamp have anything to do with Winamp, Freeamp, etc?

A: No. I was actually rather surprised when I saw how the acronym was
going to end up.

Q: Where can I get Jinamp?

A: https://github.com/bmerry/jinamp

Q: How do I install it?

A: Read the file INSTALL for generic instructions. By default a script
called playaudio is installed with jinamp, and is used as the default
player. It detects mp3 and ogg from the extension and calls mpg123 or
ogg123. It also extracts the ID3 tag or Ogg comment and displays the
artist and title on tty10.

If you want to use something different, you have a number of options:

- Edit the playaudio script to do what you want. This is the
recommended way to do things.
- Run the configure script with --with-player=<player>, where player is
your preferred player. Note the you can't specify command line options,
which is why a wrapper script like playaudio is useful. Also note that
playaudio will still be installed; delete it if you like.
- Change the player at run time, using the --player command line option
or in the configuration file.

Q: How do I use it?

A: Type `man jinamp' once you've installed it for usage instructions. If I
duplicate them here it'll become a headache to keep them in sync.

Q: Are there any bugs?

A: Probably. If you find any, you can file them on Github, but it's been over
a decade since I did any new development on jinamp so they're unlikely to get
much attention. See the man page for any known bugs.

Q: How can I contribute?

A: It's been over a decade since I did any active development, so the only way
new features are likely to appear in jinamp is if you send a pull request.

I'm a terrible manual writer so if you'd like to write an info page,
update the man page or even just a better README than this one I'd
appreciate it.

Q: Are there binaries available?

A: Unfortunately not.

Legal stuff
-----------
You may distribute jinamp under the terms of the GNU General Public Licence,
VERSION 2 ONLY.

The text of the GNU GPL can be found in the file COPYING.
