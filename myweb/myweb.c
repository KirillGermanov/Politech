#include <stdio.h>
#include <string.h>

#define HTTP_HEADER_LEN 256
#define HTTP_REQUEST_LEN 256
#define HTTP_METHOD_LEN 6
#define HTTP_URI_LEN 100

#define REQ_END 100
#define ERR_NO_URI -100
#define ERR_ENDLESS_URI -101

enum GetStates{	GET_URI,
		GET_URI_PATH,
		GET_URI_PARAMS,
		GET_VERSION
};

struct http_req {
	char request[HTTP_REQUEST_LEN];
	char method[HTTP_METHOD_LEN];
	char uri[HTTP_URI_LEN];
	char uri_path[HTTP_URI_LEN];
	char uri_params[HTTP_URI_LEN];
	char version[100];
	// user_agent
	char server[255];
	// accept
};

int processGet(char *items, char* sep, struct http_req *req)
{
	int state = GET_URI;

	while(items != NULL)
	{	
		switch(state){
			case GET_URI: state = GET_VERSION;
				char* pos = strchr(items, '/');
				if ( pos != NULL) {
					fprintf(stderr, "URI: %s\n",items);
					strncpy(req->uri, items, strlen(items));
					pos = strchr(items, '?');
					if(pos != NULL)
					{
						strncpy(req->uri_path, items, pos-items);
						fprintf(stderr, "URI_PATH: %s\n",req->uri_path);
						pos++;
						strncpy(req->uri_params, pos, &items[strlen(items)] - pos );
						fprintf(stderr, "URI_PARAMS: %s\n",req->uri_params);
					}

					break;
				}
			case GET_VERSION: fprintf(stderr, "VERSION: %s\n",items);
		 		break;

		}
			
      		items = strtok (NULL,sep);
	}

	return 0;
}

void fill_req2(char *buf, struct http_req *req)
{
	static int counter = 0;
	++counter;
	fprintf(stderr, "Process Line %i:len =%i  %s\n",counter,strlen(buf),  buf);
	
	char tmp[strlen(buf)];
	strncpy(tmp, buf, strlen(buf));

	char sep[10] = " ";
	char* items = strtok(buf,sep);
	
	//get head of line
	char head[strlen(items)];
	strncpy(head, items, strlen(items)+1);
	items = strtok (NULL,sep);

	fprintf(stderr, "HEAD: %s\n",head);

	if(!strncmp(head,"GET",3)){
		strncpy(req->request, tmp, strlen(tmp));
		strncpy(req->method, "GET", strlen("GET"));
		
		processGet(items, sep, req);	
	} else if(!strncmp(head,"Host:",5)){
		char server[strlen(items)];
		strncpy(server, items, strlen(items));
		fprintf(stderr, "Server: %s\n", server);	
	} else {
	
		while(items != NULL)
        	{
               		fprintf(stderr, "params:%i  %s\n",strlen(items), items);
              	 	items = strtok (NULL,sep);
        	}
	}

}

int fill_req(char *buf, struct http_req *req) {
	
	if (strlen(buf) == 2) {
		// пустая строка (\r\n) означает конец запроса
		return REQ_END;
	}
	
	fill_req2(buf, req);

	
	/*
	p = strstr(buf, "GET");
	if (p == buf) {
		// Строка запроса должна быть вида
		// GET /dir/ HTTP/1.0
		// GET /dir HTTP/1.1
		// GET /test123?r=123 HTTP/1.1
		// и т.п.
		strncpy(req->request, buf, strlen(buf));
		strncpy(req->method, "GET", strlen("GET"));
		a = strchr(buf, '/');
		if ( a != NULL) { // есть запрашиваемый URI 
			b = strchr(a, ' ');
			if ( b != NULL ) { // конец URI
				strncpy(req->uri, a, b-a);
			} else {
				return ERR_ENDLESS_URI;  
				// тогда это что-то не то
			}
		} else {
			return ERR_NO_URI; 
			// тогда это что-то не то
		}
	}
	*/

	return 0;	
}

int log_req(struct http_req *req) {
	fprintf(stderr, "%s %s\n%s\n", req->request, req->method, req->uri);
	return 0;
}

int make_resp(struct http_req *req) {
	printf("HTTP/1.1 200 OK\r\n");
	printf("Content-Type: text/html\r\n");
	printf("\r\n");
	printf("<html><body><title>Page title</title><h1>Page Header</h1></doby></html>\r\n");
	return 0;
}

int main (void) {
	char buf[HTTP_HEADER_LEN];
	struct http_req req;
	while(fgets(buf, sizeof(buf),stdin)) {
		int ret = fill_req(buf, &req);
		if (ret == 0) 
			// строка запроса обработана, переходим к следующей
			continue;
		if (ret == REQ_END ) 
			// конец HTTP запроса, вываливаемся на обработку
			break;
		else
			// какая-то ошибка 
			printf("Error: %d\n", ret);
		
	}
	//log_req(&req);
	make_resp(&req);
}
