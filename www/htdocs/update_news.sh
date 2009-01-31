#!/bin/sh

# Update the news on home page from SourceForge project news
/usr/bin/wget -q -O anjuta_news.html "http://sourceforge.net/export/projnews.php?group_id=14222&limit=5&flat=0&show_summaries=1"
