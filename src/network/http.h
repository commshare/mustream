# ifndef __HTTP_HANDLERS__
# define __HTTP_HANDLERS__

typedef struct HTTP_Request * HTTPRequest;
typedef struct HTTP_Response * HTTPResponse;

int   read_request      (int, HTTPRequest*);
void  print_request     (char *, HTTPRequest);
int   form_response     (HTTPRequest, HTTPResponse*);
int   write_response    (int, HTTPResponse);
void  print_response    (char *, HTTPResponse);
void  transaction_done  (HTTPRequest, HTTPResponse);


# endif

