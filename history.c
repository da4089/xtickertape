/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1995.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, Gehrmann Labs
     University of Queensland
     St Lucia, 4072
     Australia
     Tel: +61 7 3365 4310
     Fax: +61 7 3365 4311
     Email: enquiries@dstc.edu.au

  This software is being provided "AS IS" without warranty of
  any kind.  In no event shall DSTC Pty Ltd be liable for
  damage of any kind arising out of or in connection with
  the use or performance of this software.

****************************************************************/

#ifndef lint
static const char cvsid[] = "$Id: history.c,v 1.1 1999/08/15 05:37:12 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include "Message.h"
#include "history.h"

/* The structure of a node in a message history */
typedef struct history_node *history_node_t;
struct history_node
{
    /* The node's message */
    Message message;

    /* The node's `color' */
    int color;

    /* The previous node (by order of receipt) */
    history_node_t previous;

    /* The previous response (to the receiver's parent) node */
    history_node_t previous_response;

    /* The last response to the receiver */
    history_node_t last_response;
};


/* Allocates and initializes a new history_node_t */
static history_node_t history_node_alloc(Message message)
{
    history_node_t self;

    /* Allocate memory for the new node */
    if ((self = (history_node_t) malloc(sizeof(struct history_node))) == NULL)
    {
	return NULL;
    }

    /* Initialize the fields to sane values */
    self -> message = message;
    self -> color = 0;
    self -> previous = NULL;
    self -> previous_response = NULL;
    self -> last_response = NULL;
    return self;
}

/* Releases the resources consumed by the receiver */
static void history_node_free(history_node_t self)
{
    free(self);
}

/* Prints the receiver to the output file */
static void history_node_print(history_node_t self, int indent, FILE *out)
{
    int i;

    for (i = 0; i < indent; i++)
    {
	fprintf(out, "  ");
    }

    fprintf(out,
	    "%s:%s:%s\n",
	    Message_getGroup(self -> message),
	    Message_getUser(self -> message),
	    Message_getString(self -> message));
}

/* Prints the history list ending with the receiver */
static void history_node_print_all(history_node_t self, int indent, FILE *out)
{
    /* Watch for the beginning of history */
    if (self == NULL)
    {
	return;
    }

    /* Print all of the previous nodes followed by this one */
    history_node_print_all(self -> previous, indent, out);
    history_node_print(self, indent, out);
}

/* Prints a thread ending with the receiver */
static void history_node_print_threaded2(history_node_t self, int indent, FILE *out)
{
    /* Watch for the beginning of history */
    if (self == NULL)
    {
	return;
    }

    /* Print the previous responses to the receiver's parent */
    history_node_print_threaded2(self -> previous_response, indent, out);

    /* Print the receiver */
    if (self -> color == 1)
    {
	self -> color = 0;
	history_node_print(self, indent, out);

	/* Print responses to the receiver */
	history_node_print_threaded2(self -> last_response, indent + 1, out);
    }
}

/* Prints the threaded history list ending with the receiver */
static void history_node_print_threaded(history_node_t self, int indent, FILE *out)
{
    /* Watch for the beginning of history */
    if (self == NULL)
    {
	return;
    }

    /* Mark ourselves as needing to be printed */
    self -> color = 1;

    /* Print the previous nodes */
    history_node_print_threaded(self -> previous, indent, out);

    /* If we haven't been printed then do so now */
    if (self -> color == 1)
    {
	self -> color = 0;
	history_node_print(self, indent, out);

	/* Print responses to the receiver */
	history_node_print_threaded2(self -> last_response, indent + 1, out);
    }
}



/* The structure of a threaded (or not) message history */
struct history
{
    /* The number of nodes in the history */
    int count;

    /* The last node in the history (in receipt order) */
    history_node_t last;

    /* The last node in the history which isn't a response */
    history_node_t thread_last;
};


/* Allocates and initializes a new empty history */
history_t history_alloc()
{
    history_t self;

    /* Allocate memory for the new node */
    if ((self = (history_t) malloc(sizeof(struct history))) == NULL)
    {
	return NULL;
    }

    /* Initialize the fields to sane values */
    self -> count = 0;
    self -> last = NULL;
    return self;
}

/* Frees the history */
void history_free(history_t self)
{
    history_node_t probe;
    
    /* Free each node */
    probe = self -> last;
    while (probe != NULL)
    {
	history_node_t previous = probe -> previous;
	history_node_free(probe);
	probe = previous;
    }

    free(self);
}

/* Finds the node whose Message has the given id */
static history_node_t find_by_id(history_t self, long id)
{
    history_node_t node;

    /* Search the history from youngest to oldest */
    for (node = self -> last; node != NULL; node = node -> previous)
    {
	/* Return if we have a match */
	if (Message_getId(node -> message) == id)
	{
	    return node;
	}
    }

    /* Not found */
    return NULL;
}

/* Adds a message to the end of the history */
int history_add(history_t self, Message message)
{
    history_node_t node, parent;
    long reply_id;

    /* Allocate a node to hold the message */
    if ((node = history_node_alloc(message)) == NULL)
    {
	return -1;
    }

    /* Append the node to the end of the history */
    self -> count++;
    node -> previous = self -> last;
    self -> last = node;

    /* If the message isn't a response then we're done */
    if ((reply_id = Message_getReplyId(message)) == 0)
    {
	return 0;
    }

    /* See if we have the parent message in the history */
    if ((parent = find_by_id(self, reply_id)) == NULL)
    {
	return 0;
    }

    /* Append this node to the end of the parent's replies */
    node -> previous_response = parent -> last_response;
    parent -> last_response = node;

    return 0;
}


/* Just for debugging */
void history_print(history_t self, FILE *out)
{
    /* Fob this off on the nodes */
    fprintf(out, "unthreaded (%d):\n", self -> count);
    history_node_print_all(self -> last, 1, out);
}

void history_print_threaded(history_t self, FILE *out)
{
    fprintf(out, "threaded (%d):\n", self -> count);

    /* Fob this off on the nodes */
    history_node_print_threaded(self -> last, 1, out);
}
