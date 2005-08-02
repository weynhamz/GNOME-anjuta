/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/* 
 * svn-backend.c (c) 2005 Naba kumar
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <subversion-1/svn_auth.h>
#include <subversion-1/svn_pools.h>
#include "svn-backend-priv.h"
#include "svn-auth.h"
#include "plugin.h"

/* User authentication prompts handlers */
static svn_error_t*
svn_auth_simple_prompt_func_cb (svn_auth_cred_simple_t **cred, void *baton,
								const char *realm, const char *username,
								svn_boolean_t may_save, apr_pool_t *pool)
{
	/*SVN *svn = (SVN*)baton;*/
	
	/* Prompt the user for username && password.
	 * No need to be thread safe at this point as we are in the main thread.
	 * The svn thread will start later!
	 * 
	 * We must prompt the user at this point, pre-filling the username field
	 * with the passed 'username' string. The dialog will have 'Remeber
	 * password' checkbox, which should be insensitive if the passed 'may_save'
	 * value is FALSE.
	 */ 
	 
	
	GladeXML* gxml = glade_xml_new(GLADE_FILE, "svn_user_auth", NULL);
	GtkWidget* svn_user_auth = glade_xml_get_widget(gxml, "svn_user_auth");
	GtkWidget* username_entry = glade_xml_get_widget(gxml, "username_entry");
	GtkWidget* password_entry = glade_xml_get_widget(gxml, "password_entry");
	GtkWidget* remember_pwd = glade_xml_get_widget(gxml, "remember_pwd");
		
	if (!may_save)
		gtk_widget_set_sensitive(remember_pwd, FALSE);
		
	/* Then the dialog is prompted to user and when user clicks ok, the
	 * values entered, i.e username, password and remember password (true
	 * by default) should be used to initialized the memebers below. If the
	 * user cancels the dialog, I think we return an error struct
	 * appropriately initialized. -- naba
	 */
	 
 	switch (gtk_dialog_run(GTK_DIALOG(svn_user_auth)))
	{
		case GTK_RESPONSE_OK:
			*cred = apr_pcalloc (pool, sizeof(*cred));
			(*cred)->username = g_strdup(gtk_entry_get_text(GTK_ENTRY(username_entry)));
			(*cred)->password = g_strdup(gtk_entry_get_text(GTK_ENTRY(password_entry)));
			(*cred)->may_save = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remember_pwd));
			break;
		default:
			break;
	}
	return NULL;
}

#if 0
static svn_error_t*
svn_auth_username_prompt_func_cb (svn_auth_cred_username_t **cred,
								  void *baton, const char *realm,
								  svn_boolean_t may_save, apr_pool_t *pool)
{
	SVN *svn = (SVN*)baton;
	
	/* Prompt for username only. This provider may not be necessary. */
	return NULL;
}
#endif

static svn_error_t*
svn_auth_ssl_server_trust_prompt_func_cb (svn_auth_cred_ssl_server_trust_t **cred,
										  void *baton, const char *realm,
										  apr_uint32_t failures,
										  const svn_auth_ssl_server_cert_info_t *cert_info,
										  svn_boolean_t may_save,
										  apr_pool_t *pool)
{
	SVN *svn = (SVN*)baton;
	
	/* Ask user if he trusts this server. Optionally, can ask for some accepted
	 * trust failures, that when encountered later, will not bring up this
	 * prompt again.
	 */
	return NULL;
}

static svn_error_t*
svn_auth_ssl_client_cert_prompt_func_cb (svn_auth_cred_ssl_client_cert_t **cred,
										 void *baton, const char *realm,
										 svn_boolean_t may_save,
										 apr_pool_t *pool)
{
	SVN *svn = (SVN*)baton;
	
	/* Ask for the file where client certificate of authenticity is.
	 * I think it is some sort of private key. */
	return NULL;
}

static svn_error_t*
svn_auth_ssl_client_cert_pw_prompt_func_cb (svn_auth_cred_ssl_client_cert_pw_t **cred,
										   void *baton, const char *realm,
										   svn_boolean_t may_save,
										   apr_pool_t *pool)
{
	SVN *svn = (SVN*)baton;
	
	/* Prompt for password only. I think it is pass-phrase of the above key. */
	return NULL;
}

void
svn_fill_auth(SVN* svn)
{
	svn_auth_baton_t *auth_baton;

    apr_array_header_t *providers
      = apr_array_make (svn->pool, 1, sizeof (svn_auth_provider_object_t *));

    svn_auth_provider_object_t *provider;
	
	/* Provider that authenticates username/password from ~/.subversion */
	provider = apr_pcalloc (svn->pool, sizeof(*provider));
    svn_client_get_simple_provider (&provider, svn->pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = provider;

	/* Provider that authenticates server trust from ~/.subversion */
	provider = apr_pcalloc (svn->pool, sizeof(*provider));
    svn_client_get_ssl_server_trust_file_provider (&provider, svn->pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	/* Provider that authenticates client cert from ~/.subversion */
	provider = apr_pcalloc (svn->pool, sizeof(*provider));
    svn_client_get_ssl_client_cert_file_provider (&provider, svn->pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	/* Provider that authenticates client cert password from ~/.subversion */
	provider = apr_pcalloc (svn->pool, sizeof(*provider));
    svn_client_get_ssl_client_cert_pw_file_provider (&provider, svn->pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	/* Provider that prompts for username/password */
	provider = apr_pcalloc (svn->pool, sizeof(*provider));
    svn_client_get_simple_prompt_provider(&provider,
										  svn_auth_simple_prompt_func_cb,
										  svn, 3, svn->pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = provider;

	/* Provider that prompts for server trust */
	provider = apr_pcalloc (svn->pool, sizeof(*provider));
    svn_client_get_ssl_server_trust_prompt_provider (&provider,
								  svn_auth_ssl_server_trust_prompt_func_cb,
													svn, svn->pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	/* Provider that prompts for client certificate file */
	provider = apr_pcalloc (svn->pool, sizeof(*provider));
    svn_client_get_ssl_client_cert_prompt_provider (&provider,
								  svn_auth_ssl_client_cert_prompt_func_cb,
													svn, 3, svn->pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
	/* Provider that prompts for client certificate file password */
	provider = apr_pcalloc (svn->pool, sizeof(*provider));
    svn_client_get_ssl_client_cert_pw_prompt_provider (&provider,
								  svn_auth_ssl_client_cert_pw_prompt_func_cb,
													svn, 3, svn->pool);
    *(svn_auth_provider_object_t **)apr_array_push (providers) = provider;
	
    svn_auth_open (&auth_baton, providers, svn->pool);

    svn->ctx->auth_baton = auth_baton;
}
