QBedReader
==========

A text to speech reader for Wikipedia

I initially created this to give me something to listen to while falling asleep. It works by downloading a page from Wikipedia (specified either by specifying the URL or a search term).

The text of the article, when downloaded, will appear on the right, while links from that page will be added to the left. If auto read is selected, tehn a random link from that list is chosen and read. You can manually specify which articles are to be read first by selecting them.

This program depends on flite to read text. If it is on Windows, it expects "flite.exe" to be in the same directory. If it is on Linux then it expects "flite" to be a valid command.
