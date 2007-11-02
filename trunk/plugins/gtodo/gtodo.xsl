<!--

Copyright (C) 2004 Peter J Jones (pjones@pmade.org)
All Rights Reserved

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.
3. Neither the name of the Author nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

-->
<!--=================================================================================================-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<!--=================================================================================================-->
	<xsl:output method="html" indent="yes"/>
	<!--=================================================================================================-->
	<xsl:param name="title"	    select="'To Do List'"/>
	<xsl:param name="category"  select="''"/>
	<xsl:param name="css"	    select="''"/>
	<!--=================================================================================================-->
	<xsl:template match="/">
		<html>
			<head>
				<title><xsl:value-of select="$title"/></title>

				<xsl:if test="string-length($css) != 0">
					<xsl:choose>
						<xsl:when test="$css='embed'">
							<style type="text/css">


								body 
								{
								background-color: #5365a0;
								font-color: #000000;
								}

								.gtodo-page
								{
								width: 600px;
								background-color: #e2e2e2;
								display: block;
								border: solid 3px #000000;
								text-align: left;
								margin: 4px 2px 4px 8px;
								}

								.gtodo-category 
								{
								font-size: 120%;
								font-weight: bold;
								font-stretch: wider;
								margin: 4px 2px 1px 6px;
								}

								.gtodo-category-table
								{
								width: 592px;
								background-color: #e2e2e2;
								margin: 1px 4px 8px 4px;
								border: solid 1px #a1a5a9;
								}

								.gtodo-category-header
								{
								background-color: #f1f1f1;
								font-color: #000000;
								font-size: 105%;
								font-weight: bold;
								}

								.gtodo-category-header-number
								{
								text-align: right;
								}

								.gtodo-category-item-number
								{
								text-align: right;
								font-weight: 600;
								}

								.gtodo-category-header-priority,
								.gtodo-category-header-duedate,
								.gtodo-category-item-priority,
								.gtodo-category-item-duedate
								{
								text-align: center;
								}

								.gtodo-category-item-summary a 
								{
								display: block;
								width: 100%;
								color: #000000;
								text-decoration: none;
								}


								.gtodo-category-item-summary a:hover
								{
								background-color: #b9c3e3;
								cursor: help;
								}

								.gtodo-category-item-even
								{
								background-color: #ffffff;
								}

								.gtodo-category-item-odd
								{
								background-color: #edf3fe;
								}

								.gtodo-category-item-done
								{
								text-decoration: line-through;
								color: #989898;
								}

								.gtodo-category-item-done a
								{
								color: #989898;
								}


							</style>
						</xsl:when>
						<xsl:otherwise>

							<link href="{$css}" rel="stylesheet" type="text/css"/>
						</xsl:otherwise>
					</xsl:choose>
				</xsl:if>
			</head>

			<body>
				<xsl:apply-templates/>
			</body>
		</html>
	</xsl:template>
	<!--=================================================================================================-->
	<xsl:template match="gtodo">
		<xsl:choose>
			<xsl:when test="string-length($category) != 0">
				<xsl:apply-templates select="category[@title = $category]"/>
			</xsl:when>

			<xsl:otherwise>
				<xsl:for-each select="category">
					<xsl:if test="count(item) &gt; 0">
						<xsl:apply-templates select="."/>
					</xsl:if>
				</xsl:for-each>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>
	<!--=================================================================================================-->
	<xsl:template match="category">
		<div class="gtodo-page">
			<div class="gtodo-category">
				<xsl:value-of select="@title"/>
			</div>

			<table class="gtodo-category-table">
				<tr class="gtodo-category-header">
					<td class="gtodo-category-header-number">Item</td>
					<td class="gtodo-category-header-priority">Priority</td>
					<td class="gtodo-category-header-duedate">Due Date</td>
					<td class="gtodo-category-header-summary">Summary</td>
				</tr>

				<xsl:apply-templates select="item">
					<xsl:sort select="attribute/@enddate" data-type="number"/>
				</xsl:apply-templates>
			</table>
		</div>
	</xsl:template>
	<!--=================================================================================================-->
	<xsl:template match="item">
		<!-- check to see if this is an odd or even row -->
		<xsl:variable name="class-row">
			<xsl:choose>
				<xsl:when test="position() mod 2">gtodo-category-item-even</xsl:when>
				<xsl:otherwise>gtodo-category-item-odd</xsl:otherwise>
			</xsl:choose>
		</xsl:variable>

		<!-- check the status of this item -->
		<xsl:variable name="class-done">
			<xsl:choose>
				<xsl:when test="attribute/@done = 1">gtodo-category-item-done</xsl:when>
				<!-- might want to do an overdue check here -->
				<xsl:otherwise></xsl:otherwise>
			</xsl:choose>
		</xsl:variable>

		<tr class="{concat($class-row, ' ', $class-done)}">
			<td class="gtodo-category-item-number">
				<xsl:value-of select="concat(position(), '.')"/>
			</td>

			<td class="gtodo-category-item-priority">
				<xsl:choose>
					<xsl:when test="attribute/@priority = 0">Low</xsl:when>
					<xsl:when test="attribute/@priority = 1">Medium</xsl:when>
					<xsl:when test="attribute/@priority = 2">High</xsl:when>
					<xsl:otherwise>None</xsl:otherwise>
				</xsl:choose>
			</td>

			<td class="gtodo-category-item-duedate">
				<xsl:choose>
					<xsl:when test="string-length(attribute/@enddate) = 0">
						no due date	
					</xsl:when>
					<xsl:when test="attribute/@enddate = 99999999">
						no due date	
					</xsl:when>
					<xsl:otherwise>
						<xsl:call-template name="date2string">
							<xsl:with-param name="date" select="attribute/@enddate"/>
						</xsl:call-template>
					</xsl:otherwise>
				</xsl:choose>
			</td>

			<td class="gtodo-category-item-summary">
				<a href="javascript:alert('{comment}');" title="{comment}">
					<xsl:apply-templates select="summary"/>
				</a>
			</td>
		</tr>
	</xsl:template>
	<!--=================================================================================================-->
	<xsl:template name="date2string">
		<xsl:param name="date"/>

		<!-- taken from glib2 gdate.c, which was taken from the Calender FAQ -->
		<xsl:variable name="A"><xsl:value-of select="$date + 1721425 + 32045"/></xsl:variable>
		<xsl:variable name="B"><xsl:value-of select="floor((4 * ($A + 36524)) div 146097) - 1"/></xsl:variable>
		<xsl:variable name="C"><xsl:value-of select="$A - floor((146097 * $B) div 4)"/></xsl:variable>
		<xsl:variable name="D"><xsl:value-of select="floor((4 * ($C + 365)) div 1461) - 1"/></xsl:variable>
		<xsl:variable name="E"><xsl:value-of select="$C - floor(((1461 * $D) div 4))"/></xsl:variable>
		<xsl:variable name="M"><xsl:value-of select="floor((5 * ($E - 1) + 2) div 153)"/></xsl:variable>

		<xsl:variable name="mon" ><xsl:value-of select="$M + 3 - (12 * floor($M div 10))"/></xsl:variable>
		<xsl:variable name="day" ><xsl:value-of select="$E - floor((153 * $M + 2) div 5)"/></xsl:variable>
		<xsl:variable name="year"><xsl:value-of select="100 * $B + $D - 4800 + floor($M div 10)"/></xsl:variable>

		<xsl:value-of select="concat(format-number($year, '0000'), '/', format-number($mon, '00'), '/', format-number($day, '00'))"/>
	</xsl:template>
	<!--=================================================================================================-->
</xsl:stylesheet>
<!--=================================================================================================-->
<!--
vim:tw=300
-->
