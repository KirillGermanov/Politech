#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

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
						strncpy(req->uri_path, items, pos-items);
						strcat(req->uri_path,"\0");
						fprintf(stderr, "URI_PATH: %s\n",req->uri_path);
						pos++;
						strncpy(req->uri_params, pos, &items[strlen(items)] - pos+1 );
						fprintf(stderr, "URI_PARAMS: %s\n",req->uri_params);
					} else {
						if(strlen(req->uri)<2)
							strcpy(req->uri_path, "/root.html");
						else
							strncpy(req->uri_path, req->uri, strlen(req->uri));	
					}

					break;
				}
				return ERR_NO_URI;

			case GET_VERSION: strncpy(req->version,items,strlen(items)); 
				fprintf(stderr, "VERSION: %s\n",req->version);
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

int make_resp(struct http_req *req, char* base_path) {
	
	int fd;
	struct stat statbuf;
        void *mmf_ptr;
	
	char res_file[FILE_NAME_LEN] = "";
	strncpy(res_file,base_path,strlen(base_path));
	strcat(res_file,req->uri_path);

	fprintf(stderr, "open file=: %s\n", res_file);		
        if ((fd=open(res_file, O_RDONLY)) < 0) {
                perror(res_file);
                return 1;
        }
	
        if (fstat(fd, &statbuf) < 0) {
                perror(res_file);
                return 1;
        }
	
        if ((mmf_ptr = mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED) {
                perror("myfile");
                return 1;
        }
	
	char http_result[100];
       	strncpy(http_result,req->version,strlen(req->version));

	strcat(res_file," 200 OK\r\n");
	write(1,http_result,strlen(http_result));
	
	char *http_contype = "Content-Type: text/html\r\n";
	write(1,http_contype,strlen(http_contype));
	
	char *header_end = "\r\n";
	write(1,header_end,strlen(header_end));
        
	if(write(1,mmf_ptr,statbuf.st_size) != statbuf.st_size) {
                perror("stdout");
                return 1;
        }
        close(fd);
        munmap(mmf_ptr,statbuf.st_size);
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
	}
	else{
		strcat(base_path,"webroot");
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
	make_resp(&req, base_path);
}
