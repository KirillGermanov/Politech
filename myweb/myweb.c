#include <stdio.h>
#include <string.h>

#define HTTP_HEADER_LEN 256
#define HTTP_REQUEST_LEN 256
#define HTTP_METHOD_LEN 6
#define HTTP_URI_LEN 100
#define FILE_NAME_LEN 1000
#define LOG_ENTRY_LEN 1000

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
						strncpy(req->uri_path, items, pos-items+1);
						fprintf(stderr, "URI_PATH: %s\n",req->uri_path);
						pos++;
						strncpy(req->uri_params, pos, &items[strlen(items)] - pos+1 );
						fprintf(stderr, "URI_PARAMS: %s\n",req->uri_params);
					}
					else
					{
						return ERR_ENDLESS_URI;
					}

					break;
				}
				return ERR_NO_URI;

			case GET_VERSION: fprintf(stderr, "VERSION: %s\n",items);
		 		break;

		}
			
      		items = strtok (NULL,sep);
	}

	return 0;
}

int processReq(char *buf, struct http_req *req)
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

	if(!strncmp(head,"GET",3)){				//GET line	
		strncpy(req->request, tmp, strlen(tmp));
		strncpy(req->method, "GET", strlen("GET"));
		
		return processGet(items, sep, req);		
	} else if(!strncmp(head,"Host:",5)){			//HOST line
		strncpy(req->server, items, strlen(items));
		fprintf(stderr, "Server: %s\n", req->server);	
	} else {						//other lines
	
		while(items != NULL)
        	{
               		fprintf(stderr, "params:%i  %s\n",strlen(items), items);
              	 	items = strtok (NULL,sep);
        	}
	}

	return 0;

}

int fill_req(char *buf, struct http_req *req) {
	
	if (strlen(buf) == 2) {
		// пустая строка (\r\n) означает конец запроса
		return REQ_END;
	}
	
	return processReq(buf, req);
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

int main (int argc, char* argv[]) {
	
	char base_path[FILE_NAME_LEN] = "";
	char log_path[FILE_NAME_LEN] = "";
	char const *log_file = "access.log";
	if ( argc > 2 ) { // задан каталог журнализации
		strncpy(base_path, argv[1], strlen(argv[1]));
		strncpy(log_path, argv[2], strlen(argv[2]));
		strcat(log_path,"/");
		strcat(base_path,"/");
	}
	strcat(log_path,log_file);
	
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
	make_resp(&req);
}
