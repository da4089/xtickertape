/* $Id */

#include "UsenetSubscription.h"
#include <stdlib.h>
#include "FileStreamTokenizer.h"
#include "StringBuffer.h"
#include "sanity.h"

#ifdef SANITY
static char *sanity_value = "UsenetSubscription";
static char *sanity_freed = "Freed UsenetSubscription";
#endif /* SANITY */

typedef enum
{
    Error = -1,
    Equals,
    NotEquals,
    LessThan,
    LessThanEquals,
    GreaterThan,
    GreaterThanEquals,
    Matches,
    NotMatches
} Operation;


/* The UsenetSubscription data type */
struct UsenetSubscription_t
{
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */

    /* The receiver's subscription expression */
    char *expression;

    /* The receiver's ElvinConnection */
    ElvinConnection connection;

    /* The receiver's ElvinConnection information */
    void *connectionInfo;

    /* The receiver's callback function */
    UsenetSubscriptionCallback callback;

    /* The receiver's callback context */
    void *context;
};



/*
 *
 * Static function headers
 *
 */
static char *TranslateField(char *field);
static Operation TranslateOperation(char *operation);
static void ReadNextTuple(FileStreamTokenizer tokenizer, StringBuffer buffer);
static void ReadNextLine(FileStreamTokenizer tokenizer, char *token, StringBuffer buffer);
static void ReadUsenetFile(FILE *file, StringBuffer buffer);
static void HandleNotify(UsenetSubscription self, en_notify_t notification);

/*
 *
 * Static functions
 *
 */

/* Answers the Elvin field which matches the field from the usenet file */
static char *TranslateField(char *field)
{
    /* Bail if no field provided */
    if (field == NULL)
    {
	return NULL;
    }

    /* Use first character of field for quick lookup */
    switch (*field)
    {
	/* email */
	case 'e':
	{
	    if (strcmp(field, "email") == 0)
	    {
		return "FROM_EMAIL";
	    }

	    break;
	}

	/* from */
	case 'f':
	{
	    if (strcmp(field, "from") == 0)
	    {
		return "FROM_NAME";
	    }

	    break;
	}

	/* keywords */
	case 'k':
	{
	    if (strcmp(field, "keywords") == 0)
	    {
		return "KEYWORDS";
	    }

	    break;
	}

	/* newsgroups */
	case 'n':
	{
	    if (strcmp(field, "newsgroups") == 0)
	    {
		return "NEWSGROUPS";
	    }

	    break;
	}

	/* subject */
	case 's':
	{
	    if (strcmp(field, "subject") == 0)
	    {
		return "SUBJECT";
	    }

	    break;
	}

	/* x-posts */
	case 'x':
	{
	    if (strcmp(field, "x-posts") == 0)
	    {
		return "CROSSPOSTS";
	    }

	    break;
	}
    }

    return NULL;
}


/* Answers the operation (index) corresponding to the given operation, or -1 if none */
static Operation TranslateOperation(char *operation)
{
    /* Bail if no operation provided */
    if (operation == NULL)
    {
	return Error;
    }

    /* Use first character of operation for quick lookup */
    switch (*operation)
    {
	/* = */
	case '=':
	{
	    if (strcmp(operation, "=") == 0)
	    {
		return Equals;
	    }

	    break;
	}

	/* != */
	case '!':
	{
	    if (strcmp(operation, "!=") == 0)
	    {
		return NotEquals;
	    }

	    break;
	}

	/* < or <= */
	case '<':
	{
	    if (strcmp(operation, "<") == 0)
	    {
		return LessThan;
	    }

	    if (strcmp(operation, "<=") == 0)
	    {
		return LessThanEquals;
	    }

	    break;
	}

	/* > or >= */
	case '>':
	{
	    if (strcmp(operation, ">") == 0)
	    {
		return GreaterThan;
	    }

	    if (strcmp(operation, ">=") == 0)
	    {
		return GreaterThanEquals;
	    }

	    break;
	}

	/* matches */
	case 'm':
	{
	    if (strcmp(operation, "matches") == 0)
	    {
		return Matches;
	    }

	    break;
	}

	/* not */
	case 'n':
	{
	    if (strcmp(operation, "not") == 0)
	    {
		return NotMatches;
	    }

	    break;
	}
    }

    return Error;
}

/* Reads the next tuple from the FileStreamTokenizer into the StringBuffer */
static void ReadNextTuple(FileStreamTokenizer tokenizer, StringBuffer buffer)
{
    char *token;
    char *field;
    Operation operation;
    char *pattern;

    /* Read the field */
    token = FileStreamTokenizer_next(tokenizer);
    if ((field = TranslateField(token)) == NULL)
    {
	fprintf(stderr, "*** Unrecognized field \"%s\"\n", token);
	exit(1);
    }

    /* Read the operation */
    token = FileStreamTokenizer_next(tokenizer);
    if ((operation = TranslateOperation(token)) == Error)
    {
	fprintf(stderr, "** Unrecognized operation \"%s\"\n", token);
	return;
    }

    /* Read the pattern */
    pattern = FileStreamTokenizer_next(tokenizer);
    if ((pattern == NULL) || (*pattern == '/') || (*field == '\n'))
    {
	fprintf(stderr, "*** Unexpected end of usenet file line\n");
	return;
    }

    /* Construct our portion of the subscription */
    StringBuffer_append(buffer, " && ");
    StringBuffer_append(buffer, field);

    switch (operation)
    {
	case Matches:
	{
	    /* Construct our portion of the subscription */
	    StringBuffer_append(buffer, " matches(\"");
	    StringBuffer_append(buffer, pattern);
	    StringBuffer_append(buffer, "\")");
	    return;
	}

	case NotMatches:
	{
	    StringBuffer_append(buffer, " !matches(\"");
	    StringBuffer_append(buffer, pattern);
	    StringBuffer_append(buffer, "\")");
	    return;
	}

	case Equals:
	{
	    StringBuffer_append(buffer, " == \"");
	    StringBuffer_append(buffer, pattern);
	    StringBuffer_append(buffer, "\"");
	    return;
	}

	case NotEquals:
	{
	    StringBuffer_append(buffer, " != \"");
	    StringBuffer_append(buffer, pattern);
	    StringBuffer_append(buffer, "\"");
	    return;
	}

	case LessThan:
	{
	    StringBuffer_append(buffer, " < ");
	    StringBuffer_append(buffer, pattern);
	    return;
	}

	case LessThanEquals:
	{
	    StringBuffer_append(buffer, " <= ");
	    StringBuffer_append(buffer, pattern);
	    return;
	}

	case GreaterThan:
	{
	    StringBuffer_append(buffer, " > ");
	    StringBuffer_append(buffer, pattern);
	    return;
	}

	case GreaterThanEquals:
	{
	    StringBuffer_append(buffer, " >= ");
	    StringBuffer_append(buffer, pattern);
	    return;
	}

	/* This should never get called */
	case Error:
	{
	    return;
	}
    }
}


/* Scans a non-empty, non-comment line of the usenet file and adds its 
 * contents to the usenet subscription */
static void ReadNextLine(FileStreamTokenizer tokenizer, char *token, StringBuffer buffer)
{
    /* Read the group: [not] groupname */
    if (strcmp(token, "not") == 0)
    {
	if ((token = FileStreamTokenizer_next(tokenizer)) == NULL)
	{
	    fprintf(stderr, "*** Invalid usenet line containing \"%s\"\n", token);
	    exit(1);
	}

	StringBuffer_append(buffer, "( !NEWSGROUPS matches(\"");
	StringBuffer_append(buffer, token);
	StringBuffer_append(buffer, "\")");
    }
    else
    {
	StringBuffer_append(buffer, "( NEWSGROUPS matches(\"");
	StringBuffer_append(buffer, token);
	StringBuffer_append(buffer, "\")");
    }

    /* Read tuples */
    while (1)
    {
	token = FileStreamTokenizer_next(tokenizer);

	/* End of file or line? */
	if ((token == NULL) || (*token == '\n'))
	{
	    StringBuffer_append(buffer, " )");
	    return;
	}

	/* Look for our tuple separator */
	if (*token == '/')
	{
	    ReadNextTuple(tokenizer, buffer);
	}
	else
	{
	    fprintf(stderr, "*** Invalid usenet file\n");
	    exit(1);
	}
    }
}

/* Scans the usenet file to construct a single subscription to cover
 * all of the usenet news stuff, which is written into buffer */
static void ReadUsenetFile(FILE *file, StringBuffer buffer)
{
    FileStreamTokenizer tokenizer = FileStreamTokenizer_alloc(file, " \t", "/\n");
    char *token;
    int isFirst = 1;

    /* Locate a non-empty line which doesn't begin with a '#' */
    while ((token = FileStreamTokenizer_next(tokenizer)) != NULL)
    {
	/* Comment line */
	if (*token == '#')
	{
	    FileStreamTokenizer_skipToEndOfLine(tokenizer);
	}
	/* Useful line? */
	else if (*token != '\n')
	{
	    if (isFirst)
	    {
		isFirst = 0;
	    }
	    else
	    {
		StringBuffer_append(buffer, " || ");
	    }

	    ReadNextLine(tokenizer, token, buffer);
	}
    }

    FileStreamTokenizer_free(tokenizer);
}


/* Transforms a usenet notification into a Message and delivers it */
static void HandleNotify(UsenetSubscription self, en_notify_t notification)
{
    Message message;
    StringBuffer buffer;
    en_type_t type;
    char *string;
    char *mimeType;
    char *mimeArgs;
    char *name;
    char *email;
    char *newsgroups;
    char *pointer;

    /* Get the name from the FROM field (if provided) */
    if ((en_search(notification, "FROM_NAME", &type, (void **)&name) != 0) ||
	(type != EN_STRING))
    {
	name = "anonymous";
    }

    /* Get the e-mail address (if provided) */
    if ((en_search(notification, "FROM_EMAIL", &type, (void **)&email) != 0) ||
	(type != EN_STRING))
    {
	email = "anonymous";
    }

    /* Get the newsgroups to which the message was posted */
    if ((en_search(notification, "NEWSGROUPS", &type, (void **)&newsgroups) != 0) ||
	(type != EN_STRING))
    {
	newsgroups = "news";
    }

    /* Locate the first newsgroup to which the message was posted */
    for (pointer = newsgroups; *pointer != '\0'; pointer++)
    {
	if (*pointer == ',')
	{
	    *pointer = '\0';
	}

	break;
    }

    /* Construct the USER field */
    buffer = StringBuffer_alloc();

    /* If the name and e-mail addresses are identical, just use one as the user name */
    if (strcmp(name, email) == 0)
    {
	StringBuffer_append(buffer, name);
	StringBuffer_append(buffer, ": ");
	StringBuffer_append(buffer, newsgroups);
    }
    /* Otherwise construct the user name from the name and e-mail field */
    else
    {
	StringBuffer_append(buffer, name);
	StringBuffer_append(buffer, " <");
	StringBuffer_append(buffer, email);
	StringBuffer_append(buffer, ">: ");
	StringBuffer_append(buffer, newsgroups);
    }

    /* Construct the text of the Message */

    /* Get the SUBJECT field (if provided) */
    if ((en_search(notification, "SUBJECT", &type, (void **)&string) != 0) ||
	(type != EN_STRING))
    {
	string = "[no subject]";
    }


    /* Construct the mime attachment information (if provided) */

    /* Get the MIME_ARGS field to use as the mime args (if provided) */
    if ((en_search(notification, "MIME_ARGS", &type, (void **)&mimeArgs) == 0) &&
	(type == EN_STRING))
    {
	/* Get the MIME_TYPE field (if provided) */
	if ((en_search(notification, "MIME_TYPE", &type, (void **)&mimeType) != 0) ||
	    (type != EN_STRING))
	{
	    mimeType = "x-elvin/url";
	}
    }
    /* No MIME_ARGS provided.  Use the Message-Id field */
    else if ((en_search(notification, "Message-Id", &type, (void **)&mimeArgs) == 0) &&
	    (type == EN_STRING))
    {
	mimeType = "x-elvin/news";
    }
    /* No Message-Id field provied either */
    else
    {
	mimeType = NULL;
	mimeArgs = NULL;
    }

    /* Construct a Message out of all of that */
    message = Message_alloc(
	NULL,
	"usenet", StringBuffer_getBuffer(buffer),
	string, 60,
	mimeType, mimeArgs,
	0, 0);

    /* Deliver the Message */
    (*self -> callback)(self -> context, message);
    StringBuffer_free(buffer);
}






/*
 *
 * Exported functions
 *
 */

/* Read the UsenetSubscription from the given file */
UsenetSubscription UsenetSubscription_readFromUsenetFile(
    FILE *usenet, UsenetSubscriptionCallback callback, void *context)
{
    UsenetSubscription subscription;
    StringBuffer buffer = StringBuffer_alloc();

    /* Write the beginning of our expression */
    StringBuffer_append(buffer, "ELVIN_CLASS == \"NEWSWATCHER\" && ");
    StringBuffer_append(buffer, "ELVIN_SUBCLASS == \"MONITOR\" && ( ");

    /* Get the subscription expressions for each individual line */
    ReadUsenetFile(usenet, buffer);

    /* Close off our subscription expression */
    StringBuffer_append(buffer, " )");
    subscription = UsenetSubscription_alloc(StringBuffer_getBuffer(buffer), callback, context);
    StringBuffer_free(buffer);

    return subscription;
}



/* Answers a new UsenetSubscription */
UsenetSubscription UsenetSubscription_alloc(
    char *expression,
    UsenetSubscriptionCallback callback,
    void *context)
{
    UsenetSubscription self = (UsenetSubscription) malloc(sizeof(struct UsenetSubscription_t));

#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */

    self -> expression = strdup(expression);
    self -> connection = NULL;
    self -> connectionInfo = NULL;
    self -> callback = callback;
    self -> context = context;
    return self;
}

/* Releases the resources used by an UsenetSubscription */
void UsenetSubscription_free(UsenetSubscription self)
{
    SANITY_CHECK(self);

    /* Free the receiver's expression if it has one */
    if (self -> expression)
    {
	free(self -> expression);
    }

#ifdef SANITY
    self -> sanity_check = sanity_freed;
#else /* SANITY */
    free(self);
#endif /* SANITY */
}

/* Prints debugging information */
void UsenetSubscription_debug(UsenetSubscription self)
{
    SANITY_CHECK(self);

    printf("UsenetSubscription (%p)\n", self);
#ifdef SANITY
    printf("  sanity_check = \"%s\"\n", self -> sanity_check);
#endif /* SANITY */
    printf("  expression = \"%s\"\n", self -> expression);
    printf("  connection = %p\n", self -> connection);
    printf("  connectionInfo = %p\n", self -> connectionInfo);
    printf("  callback = %p\n", self -> callback);
    printf("  context = %p\n", self -> context);
}

/* Sets the receiver's ElvinConnection */
void UsenetSubscription_setConnection(UsenetSubscription self, ElvinConnection connection)
{
    SANITY_CHECK(self);

    /* Shortcut if we're already subscribed */
    if (self -> connection == connection)
    {
	return;
    }

    /* Unsubscribe from the old connection */
    if (self -> connection != NULL)
    {
	ElvinConnection_unsubscribe(self -> connection, self -> connectionInfo);
    }

    self -> connection = connection;

    /* Subscribe to the new connection */
    if (self -> connection != NULL)
    {
	self -> connectionInfo = ElvinConnection_subscribe(
	    self -> connection, self -> expression,
	    (NotifyCallback)HandleNotify, self);
    }
}
