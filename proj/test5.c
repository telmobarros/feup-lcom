#include <stdio.h>
#include <minix/syslib.h>
#include <minix/drivers.h>
#include "vbe.h"
#include "test5.h"
#include "timer.h"
#include "kbd.h"
#include "video_gr.h"
#include "read_xpm.h"
#include "pixmap.h"
#include "i8254.h"
#include "i8042.h"
#include "sprite.h"
#include "lmlib.h"

void *test_init(unsigned short mode, unsigned short delay) {

	char *vram_adress=vg_init(mode);

	//variavel que guardara a informa��o toda relativa ao modo vbe
	vbe_mode_info_t vbe_info;
	vbe_get_mode_info(mode, &vbe_info);

	int  timer_irq_set = timer_subscribe_int();
	int ipc_status;
	message msg;
	int r;
	timer_test_square(60); //for�a o timer a trabalhar a 60HZ
	int counter = 0; //Inicializa��o do contador

	while( counter < (delay * 60)) {
		if ( (r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
			printf("driver_receive failed with: %d", r);
			continue;
		}
		if (is_ipc_notify(ipc_status)) { /* received notification */
			switch (_ENDPOINT_P(msg.m_source)) {
			case HARDWARE: /* hardware interrupt notification */
				if (msg.NOTIFY_ARG & timer_irq_set) { /* subscribed interrupt */
					counter++;
				}
			default:
				break; /* no other notifications expected: do nothing */
			}
		} else { /* received a standard message, not a notification */
			/* no standard messages expected: do nothing */
		}
	}

	timer_unsubscribe_int();

	vg_exit();

	printf("VRAM ADRESS: 0x%02x \n", vram_adress);
	printf("PHYSICAL ADRESS: 0x%02x \n", getPhysicalAdress());
	return 0;

}

void wait_for_esc(){
	int irq_set = kbd_subscribe_int();
	int ipc_status;
	message msg;
	int r;
	unsigned int code;

	while( code != BREAKCODE ) { /* You may want to use a different condition */
		/* Get a request message. */
		if ( (r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
			printf("driver_receive failed with: %d", r);
			continue;
		}
		if (is_ipc_notify(ipc_status)) { /* received notification */
			switch (_ENDPOINT_P(msg.m_source)) {
			case HARDWARE: /* hardware interrupt notification */
				if (msg.NOTIFY_ARG & irq_set) {
					code = kbd_output();
				}
				break;
			default:
				break; /* no other notifications expected: do nothing */
			}
		} else { /* received a standard message, not a notification */
			/* no standard messages expected: do nothing */
		}
	}
	kbd_unsubscribe_int();
}

int test_square(unsigned short x, unsigned short y, unsigned short size, unsigned long color) {
	vg_init(0x105);

	unsigned int i,j;
	for(i = y; i< y+size;i++){
		for(j = x; j < x+size;j++){
			vg_pixel(j,i,color);
		}
	}

	wait_for_esc();

	vg_exit();
	return 0;
}

int test_line(unsigned short xi, unsigned short yi, 
		unsigned short xf, unsigned short yf, unsigned long color) {

	/*short deltax, deltay;
	short decl;
	printf ("%g \n", decl);
	short b = yi-xi*decl;
	short tempy, tempx;
	unsigned int i;
	deltax = xf - xi;
	deltay = yf - yi;
	vg_init(0x105);


	if(deltax >= deltay){
		for(i = xi; i < xf; i++){
			tempy = i*decl + b;
			printf ("%g \n", tempy);
			vg_pixel(i, (int)tempy, color);
		}
	}
	else{
		for(i  = yi; i < yf; i++){
			tempx = (i-b)/decl;
			printf("%g \n", tempx);
			vg_pixel(i, (int)tempx, color);
		}
	}*/
	//Metodo de bresenham
	int slope;
	int dx, dy, incE, incNE, d, x, y;
	int x1, x2, y1, y2;
	x1 = xi;
	x2 = xf;
	y1 = yi;
	y2 = yf;
	if(xi < 0 || xi > 1023 ||
			xf < 0 || xf > 1023 ||
			yi < 0 || yi > 767 ||
			yf < 0 || yf > 767 ||
			color > 256)
		return -1;

	// Inverte a linha caso o xfinal seja superior ao xinicial
	if (xi > xf){
		x1 = xf;
		x2 = xi;
		y1 = yf;
		y2 = yi;
	}
	dx = x2 - x1;
	dy = y2 - y1;

	if (dy < 0){
		slope = -1;
		dy = -dy;
	}
	else{
		slope = 1;
	}
	vg_init(0x105);
	// Constante de Bresenham
	incE = 2 * dy;
	incNE = 2 * dy - 2 * dx;
	d = 2 * dy - dx;
	y = y1;
	for (x = x1; x <= x2; x++){
		vg_pixel(x, y, color);
		if (d <= 0){
			d += incE;
		}
		else{
			d += incNE;
			y += slope;
		}
	}

	wait_for_esc();

	vg_exit();
	return 0;
}

int test_xpm(unsigned short xi, unsigned short yi, char *xpm[]) {

	vg_init(0x105);

	unsigned int xpmw , xpmh; //pixmap dimensions

	char * pixmap = read_xpm(xpm, &xpmw, &xpmh);

	unsigned int i,j;
	for(i = yi; i< yi + xpmh;i++){
		for(j = xi; j < xi + xpmw;j++){
			vg_pixel(j,i,*pixmap);
			pixmap++;
		}
	}

	wait_for_esc();
	vg_exit();
	return 0;
}	


void moveSprite(Sprite * s){
	unsigned int i,j;

	for(i = s->y; i< s->y + s->height;i++){
		for(j = s->x; j < s->x + s->width;j++){
			vg_pixel(j,i,0);
		}
	}
	s->x += s->xspeed;
	s->y += s->yspeed;

	char *pixmap = s->map;
	for(i = s->y; i< s->y + s->height;i++){
		for(j = s->x; j < s->x + s->width;j++){
			vg_pixel(j,i,*pixmap);
			pixmap++;
		}
	}
}

int test_move(unsigned short xi, unsigned short yi, char *xpm[], 
		unsigned short hor, short delta, unsigned short time) {

	vg_init(0x105);


	Sprite sprite;
	sprite.map = read_xpm(xpm, &sprite.width,&sprite.height);
	sprite.x = xi;
	sprite.y = yi;
	if (hor){
		sprite.xspeed = delta;
		sprite.yspeed = 0;
	}
	else{
		sprite.xspeed = 0;
		sprite.yspeed = delta;
	}

	int timer_irq_set = timer_subscribe_int();
	int kbd_irq_set = kbd_subscribe_int();
	int ipc_status;
	message msg;
	int r;
	unsigned int code,counter = 0;
	timer_test_square(60);

	while( code != BREAKCODE  ) { /* You may want to use a different condition */
		/* Get a request message. */
		if ( (r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
			printf("driver_receive failed with: %d", r);
			continue;
		}
		if (is_ipc_notify(ipc_status)) { /* received notification */
			switch (_ENDPOINT_P(msg.m_source)) {
			case HARDWARE: /* hardware interrupt notification */
				if (msg.NOTIFY_ARG & kbd_irq_set) {
					code = kbd_output();
				}
				if (msg.NOTIFY_ARG & timer_irq_set && counter < time* 60) {
					counter++;
					moveSprite(&sprite);

				}
				break;
			default:
				break; /* no other notifications expected: do nothing */
			}
		} else { /* received a standard message, not a notification */
			/* no standard messages expected: do nothing */
		}
	}
	timer_unsubscribe_int();
	kbd_unsubscribe_int();

	vg_exit();

}					

int test_controller() {
	mmap_t map;

	lm_init();
	lm_alloc(sizeof(vbe_controller_info_t), &map);

	vbe_get_controller_info((vbe_controller_info_t *)map.phys);

	vbe_controller_info_t *controller_info = map.virtual;

	printf("Capabilities: 0x%X\n", controller_info->Capabilities);

	printf("VRAM Total Memory: %d kb\n", controller_info->TotalMemory);

	lm_free(&map);

	return 0;
}					
