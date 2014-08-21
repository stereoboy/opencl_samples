/*
 * simple png file handling sample
 */

#include<stdio.h>
#include<stdlib.h>
#include<png.h>
#include<gtk/gtk.h>

#define LOG(fmt, args...)  \
	fprintf(stderr, "[LOG %s:%d] " fmt, __func__, __LINE__, ##args);

static png_struct *png = NULL;
static png_info *info = NULL;
static unsigned char *image_data = NULL;
const char *get_color_type_str(int index)
{
	switch(index)
	{
	case PNG_COLOR_TYPE_GRAY:
		return "PNG_COLOR_TYPE_GRAY";
	case PNG_COLOR_TYPE_PALETTE:
		return "PNG_COLOR_TYPE_PALETTE";
	case PNG_COLOR_TYPE_RGB:
		return "PNG_COLOR_TYPE_RGB";
	case PNG_COLOR_TYPE_RGB_ALPHA:
		return "PNG_COLOR_TYPE_RGB_ALPHA";
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		return "PNG_COLOR_TYPE_GRAY_ALPHA";
	default:
		return "UNKNOW_TYPE";
	}
}

void readpng_version_info()
{
	LOG("PNG_LIBPNG_VER_STRING=%s, png_libpng_ver=%s\n", PNG_LIBPNG_VER_STRING, png_libpng_ver);
	LOG("ZLIB_VERSION=%s, zlib_version=%s\n", ZLIB_VERSION, zlib_version);

}

int readpng_init(FILE *fp, long *pwidth, long *pheight, int *pcolor_type)
{
	unsigned char sig[8];
	png_uint_32 width, height;
	int	bit_depth, color_type;

	fread(sig, 1, 8, fp);
	if(!png_check_sig(sig, 8))
		return 0;
	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png)
	{
		return 0;
	}

	info = png_create_info_struct(png);
	if(!info)
	{
		png_destroy_read_struct(&png, NULL, NULL);
		return 0;
	}

	png_init_io(png, fp);
	png_set_sig_bytes(png, 8);
	png_read_info(png, info);

	png_get_IHDR(png, info, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
	LOG("Resolution: %ldx%ld\n", width, height);
	LOG("bit_depth=%d, color_type=%s\n", bit_depth, get_color_type_str(color_type));

	*pwidth = width;
	*pheight = height;
	*pcolor_type = color_type;
	return 1;
}

unsigned char *readpng_get_image(int color_type, int width, int height)
{
	int i;
	png_uint_32 rowbytes;
	png_bytep *row_ptrs;
	unsigned char *image_data;

	if(color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_expand(png);
	else if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png);

	png_read_update_info(png, info);
	{
		png_uint_32 width, height;
		int	bit_depth, color_type;
		png_get_IHDR(png, info, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);
		LOG("Resolution: %ldx%ld\n", width, height);
		LOG("bit_depth=%d, color_type=%s\n", bit_depth, get_color_type_str(color_type));
	}

	rowbytes = png_get_rowbytes(png, info);

	LOG("Rowbytes = %ld\n", rowbytes);

	row_ptrs = (png_bytep *)malloc(sizeof(png_bytep)*height);
	image_data = (unsigned char *)malloc(rowbytes*height);

	for(i = 0; i < height; i++)
	{
		row_ptrs[i] = image_data + i*rowbytes;
	}

	png_read_image(png, row_ptrs);
	png_read_end(png, NULL);
	free(row_ptrs);
	return  image_data;
}
void readpng_cleanup()
{
	if (png && info)
	{
		png_destroy_read_struct(&png, &info, NULL);
		png = NULL;
		info = NULL;
	}

	LOG("Cleanup is complete.\n");
}

int loadImage(GtkWidget *image, unsigned char *data, int width, int height)
{
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data(data, GDK_COLORSPACE_RGB, FALSE, 8, width, height, width*3, NULL, NULL);
	gtk_image_set_from_pixbuf((GtkImage *) image, pixbuf);
	gtk_widget_queue_draw(image);
}

int main(int argc, char* argv[])
{
	readpng_version_info();
	FILE *fp = NULL;
	long width, height;
	int color_type;
	unsigned char* image_data = NULL;
	GtkWidget *image;
	GtkWidget *window;

	fp = fopen(argv[1], "r+");
	readpng_init(fp, &width, &height, &color_type);
	image_data = readpng_get_image(color_type, width, height);

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Image2");
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

	gtk_container_set_border_width(GTK_CONTAINER(window), 2);

	image = gtk_image_new();

	gtk_container_add(GTK_CONTAINER(window), image);

	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

	gtk_widget_show_all(window);

	loadImage(image, image_data, width, height);

	gtk_main();

	readpng_cleanup();
	free(image_data);
	return 0;
}
