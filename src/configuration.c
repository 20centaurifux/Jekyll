/***************************************************************************
    begin........: April 2009
    copyright....: Sebastian Fedrau
    email........: lord-kefir@arcor.de
 ***************************************************************************/

/***************************************************************************
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.
 ***************************************************************************/
/*!
 * \file configuration.c
 * \brief Managing configuration data.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 10. May 2010
 */

#include <string.h>
#include <stdlib.h>

#include "configuration.h"

/**
 * @addtogroup Core
 * @{
 * 	@addtogroup Config
 * 	@{
 */

enum
{
	PROP_0,
	PROP_FILENAME,
	PROP_ROOT
};

/**
 * \enum _ConfigElement
 * \brief Element types.
 */
typedef enum
{
	/*! Found element should be ignored. */
	PARSER_ELEMENT_IGNORE,
	/*! Found element is a section. */
	PARSER_ELEMENT_SECTION,
	/*! Found element is a value. */
	PARSER_ELEMENT_VALUE,
	/*! Found element is an array item. */
	PARSER_ELEMENT_ITEM
} _ConfigElement;

/**
 * \struct _ConfigSerializationData
 * \brief Data required for serialization of configuration data.
 */
typedef struct
{
	/*! Output channel. */
	GIOChannel *channel;
	/*! Pointer to a GError structure. */
	GError **err;
	/*! Status of the serialization process. */
	gboolean success;
	
} _ConfigSerializationData;

/**
 * \struct _ConfigDeserializationData
 * \brief Data required for deserialization of configuration data.
 */
typedef struct
{
	/*! A stack containing sections. */
	GSList *sections;
	/*! TRUE until the first section has been appended. */
	gboolean first_section;
	/*! Type of the found element. */
	_ConfigElement element;
	/*! Holds the datatype of the current element (only if the element is a value). */
	ValueDatatype datatype;
	/*! The last created value. */
	Value *value;
	/**
	 * \struct _array
	 * \brief Holds array data, length and element size.
	 *
	 * \var array
	 * \brief Array data.
	 */
	struct _array
	{
		/*! Array containing found items. */
		gpointer data;
		/*! Current length of the array. */
		guint32 length;
		/*! Allocated size of the array. */
		guint32 size;
		/*! Size of the items. */
		gsize element_size; 
	} array;
	/*! Holds error information */
	GError *err;                
} _ConfigDeserializationData;

/*! Minimum size of allocated blocks. */
#define CONFIG_PARSER_ARRAY_BLOCK_SIZE 10

static GObjectClass *parent_class = NULL;

/* translation table for the ValueDatatype enumeration */
static const gchar *datatype_table[] =
{
	"integer", "float", "bool", "string", "integer[]", "float[]", "bool[]", "string[]"
};

static void config_class_init(ConfigClass *klass);
static void config_constructor(Config *config);
static void config_finalize(GObject *obj);
static void _config_set_property(GObject *object, guint property_id, const GValue *config, GParamSpec *pspec);
static void _config_get_property(GObject *object, guint property_id, GValue *config, GParamSpec *pspec);

/* serialization */
static gboolean _config_serialize(GIOChannel *channel, Section *root, GError **err);
static void _config_serialize_section(Section *section, gpointer user_data);
static void _config_serialize_value(Value *value, gpointer user_data);

/* deserialization */
static gboolean _config_parse_data(Section *root, const gchar *data, gsize len, GError **err);
static gboolean _config_parse_section(const gchar **attribute_names, const gchar **attribute_values, const gchar **section);
static gboolean _config_parse_value(const gchar **attribute_names, const gchar **attribute_values, const gchar **name, ValueDatatype *datatype);
static void _config_process_section_data(_ConfigDeserializationData *param, const gchar *name);
static void _config_process_value_data(_ConfigDeserializationData *param, const gchar *name, ValueDatatype datatype);
static void _config_parse_data_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names,
                                             const gchar **attribute_values, gpointer user_data, GError **err);
static void _config_parse_data_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error);
static void _config_parse_data_end_element(GMarkupParseContext *context, const char *element_name, gpointer user_data, GError **err);

/* methods */
static Section *_config_get_root(Config *config);
static gboolean _config_load(Config *config, const gchar *filename, GError **err);
static gboolean _config_save(Config *config, GError **err);
static gboolean _config_save_as(Config *config, const gchar *filename, GError **err);
static const gchar *_config_get_filename(Config *config);

/*
 *	initialization/finalization:
 */
GType
config_get_type(void)
{
	static GType config_type = 0;

	g_type_init();

	if(!config_type)
	{
		static const GTypeInfo config_type_info =
		{
			sizeof(ConfigClass),
			NULL,
			NULL,
			(GClassInitFunc)config_class_init,
			NULL,
			NULL,
			sizeof(Config),
			0,
			(GInstanceInitFunc)config_constructor
		};
		config_type = g_type_register_static(G_TYPE_OBJECT, "Config", &config_type_info, (GTypeFlags)0);
	}

	return config_type;
}

static void
config_class_init(ConfigClass *klass)
{
	GObjectClass *object_class;

	parent_class = (GObjectClass *)g_type_class_peek_parent(klass);
	object_class = (GObjectClass *)klass;

	/* overwrite funtions */
	object_class->finalize = &config_finalize;
	object_class->set_property = &_config_set_property;
	object_class->get_property = &_config_get_property;

	/* class methods */
	klass->get_root = &_config_get_root;
	klass->get_filename = &_config_get_filename;
	klass->load = &_config_load;
	klass->save = &_config_save;
	klass->save_as = &_config_save_as;

	/* install properties */
	g_object_class_install_property(object_class,
		                        PROP_FILENAME,
		                        g_param_spec_string("filename", NULL, NULL, NULL, G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
		                        PROP_ROOT,
		                        g_param_spec_object("root", NULL, NULL, SECTION_TYPE, G_PARAM_READABLE));
}

static void
config_constructor(Config *config)
{
	config->priv.filename = NULL;
	config->priv.root = section_new("config");
}

static void
config_finalize(GObject *obj)
{
	Config *self = (Config *)obj;

	G_OBJECT_CLASS(parent_class)->finalize(obj);
	g_assert(self->priv.root != NULL);

	if(self->priv.filename)
	{
		g_free(self->priv.filename);
	}

	if(self->priv.root)
	{
		section_unref(self->priv.root);
	}
}

/*
 *	serialization:
 */
static gboolean
_config_serialize(GIOChannel *channel, Section *root, GError **err)

{
	gboolean success = FALSE;
	_ConfigSerializationData *data;

	g_debug("[SERIALIZATION STARTED]");

	g_return_val_if_fail(root != NULL, FALSE);

	/* create callback paramter */
	data = (_ConfigSerializationData *)g_malloc(sizeof(_ConfigSerializationData));
	data->channel = channel;
	data->err = err;
	data->success = TRUE;

	/* serialize data */
	_config_serialize_section(root, data);
	success = data->success;

	/* free callback parameter */
	g_free(data);

	g_debug("[SERIALIZATION COMPLETED]");

	return success;
}

static void
_config_serialize_section(Section *section, gpointer user_data)
{
	_ConfigSerializationData *data = (_ConfigSerializationData *)user_data;
	gchar *section_name;
	GString *text = g_string_sized_new(32);

	g_debug("%s: serializing section: \"%s\"", __func__, section_get_name(section));

	g_return_if_fail(section != NULL);
	g_return_if_fail(data->success == TRUE);

	/* escape section name */
	if((section_name = g_markup_escape_text(section_get_name(section), -1)))
	{
		/* write opening tag */
		g_string_printf(text, "<section name=\"%s\">\n", section_name);
		g_io_channel_write_chars(data->channel, text->str, -1, NULL, NULL);

		/* serialize values */
		section_foreach_value(section, _config_serialize_value, user_data);

		/* iterate children */
		if(data->success)
		{
			section_foreach_child(section, _config_serialize_section, user_data);
		}

		/* write closing tag */
		g_string_printf(text, "</section>\n");
		g_io_channel_write_chars(data->channel, text->str, -1, NULL, NULL);

	}
	else
	{
		data->success = FALSE;
		g_set_error(data->err, 0, 0, "Couldn't escape section name: \"%s\"", section_get_name(section));
	}
	

	/* free allocated memory */
	g_free(section_name);
	g_string_free(text, TRUE);

}

static void
_config_serialize_value(Value *value, gpointer user_data)
{
	_ConfigSerializationData *data = (_ConfigSerializationData *)user_data;
	gchar *escaped;
	GString *text = g_string_sized_new(512);
	ValueDatatype datatype;
	gconstpointer array;
	guint len;

	g_debug("%s: serializing value: \"%s\"", __func__, value_get_name(value));

	g_return_if_fail(value != NULL);
	g_return_if_fail(data->success == TRUE);

	/* get datatype */
	g_object_get(value, "datatype", &datatype, NULL);

	g_assert(datatype >= 0 && datatype <= sizeof(datatype_table) / sizeof(datatype_table[0]));

	if((escaped = g_markup_escape_text(value_get_name(value), -1)))
	{
		/* write opening tag */
		g_string_printf(text, "<value name=\"%s\" datatype=\"%s\">", escaped, datatype_table[datatype]);
		g_free(escaped);

		if(VALUE_IS_ARRAY(value))
		{
			/* convert value */
			g_string_append_printf(text, "\n<items>\n");
	
			array = value_get_array(value, &len);
			for(guint i = 0; i < len; ++i)
			{
				switch(datatype)
				{
					case VALUE_DATATYPE_INT32_ARRAY:
						g_string_append_printf(text, "<item>%d</item>\n", ((gint32 *)array)[i]);
						break;

					case VALUE_DATATYPE_FLOAT_ARRAY:
						g_string_append_printf(text, "<item>%2.2f</item>\n", ((gfloat *)array)[i]);
						break;

					case VALUE_DATATYPE_BOOLEAN_ARRAY:
						g_string_append_printf(text, "<item>%s</item>\n", ((gboolean *)array)[i] ? "true" : "false");
						break;

					case VALUE_DATATYPE_STRING_ARRAY:
						escaped = g_markup_escape_text(((gchar **)array)[i], -1);
						g_string_append_printf(text, "<item>%s</item>\n", escaped);
						g_free(escaped);
						break;

					default:
						g_log(NULL, G_LOG_LEVEL_ERROR, "Couldn't serialize value (datatype=%d)", datatype);
				}
			}

			g_string_append_printf(text, "</items>\n</value>\n");
		}
		else
		{
			switch(datatype)
			{
				case VALUE_DATATYPE_INT32:
					g_string_append_printf(text, "%d</value>\n", value_get_int32(value));
					break;

				case VALUE_DATATYPE_FLOAT:
					g_string_append_printf(text, "%2.2f</value>\n", value_get_float(value));
					break;

				case VALUE_DATATYPE_BOOLEAN:
					g_string_append_printf(text, "%s</value>\n", value_get_bool(value) ? "true" : "false");
					break;

				case VALUE_DATATYPE_STRING:
					escaped = g_markup_escape_text(value_get_string(value), -1);
					g_string_append_printf(text, "%s</value>\n", escaped);
					g_free(escaped);
					break;

				default:
					g_log(NULL, G_LOG_LEVEL_ERROR, "Couldn't serialize value (datatype=%d)", datatype);
			}
		}

		g_io_channel_write_chars(data->channel, text->str, -1, NULL, NULL);
	}
	else
	{
		data->success = FALSE;
		g_set_error(data->err, 0, 0, "Couldn't escape value name: \"%s\"", value_get_name(value));
	}

	/* free allocated memory */
	g_string_free(text, TRUE);
}

/*
 *	deserialization:
 */
static gboolean
_config_parse_data(Section *root, const gchar *data, gsize len, GError **err)
{
	GMarkupParser parser;
	GMarkupParseContext *context;
	_ConfigDeserializationData param;
	gboolean success = FALSE;

	g_debug("[DESERIALIZATION STARTED]");

	g_assert(root != NULL);
	g_return_val_if_fail(data != NULL, FALSE);

	parser.start_element = _config_parse_data_start_element;
	parser.end_element = _config_parse_data_end_element;
	parser.text = _config_parse_data_text;
	parser.passthrough = NULL;
	parser.error = NULL;

	param.sections = g_slist_append(NULL, root);
	param.first_section = TRUE;
	param.err = NULL;

	if((context = g_markup_parse_context_new(&parser, 0, &param, NULL)))
	{
		success = g_markup_parse_context_parse(context, data, len, err);

		/* free allocated memory */
		g_markup_parse_context_free(context);
	}
	else
	{
		g_set_error(err, 0, 0, "Couldn't parse configuration file.");
	}

	/* free allocated memory */
	g_slist_free(param.sections);

	g_debug("[DESERIALIZATION COMPLETED]");

	return success;
}

static gboolean
_config_parse_section(const gchar **attribute_names, const gchar **attribute_values, const gchar **section)
{
	for(guint i = 0; ; ++i)
	{
		if(!g_strcmp0(attribute_names[i], "name"))
		{
			*section = attribute_values[i];
			return TRUE;
		}
	}

	return FALSE;
}

static gboolean
_config_parse_value(const gchar **attribute_names, const gchar **attribute_values, const gchar **name, ValueDatatype *datatype)
{
	guint i = 0;
	guint counter = 0;
	static guint length = 0;

	while(attribute_names[i])
	{
		if(!g_strcmp0(attribute_names[i], "name"))
		{
			*name = attribute_values[i];
			++counter;
		}
		else
		{
			if(!g_strcmp0(attribute_names[i], "datatype"))
			{
				/* try to convert string into datatype enumeration */
				if(!length)
				{
					length = sizeof(datatype_table) / sizeof(gchar *);
				}

				for(guint type = 0; type < length; ++type)
				{
					if(!g_strcmp0(datatype_table[type], attribute_values[i]))
					{
						*datatype = type;
						++counter;
					}
				}
			}
		}

		++i;
	}

	return (counter == 2) ? TRUE : FALSE;;
}

static void
_config_process_section_data(_ConfigDeserializationData *param, const gchar *name)
{
	GSList *element;
	Section *parent;
	Section *child;

	g_assert(param != NULL);
	g_return_if_fail(param->err == NULL);

	/* check if the current section is the root section */
	if(param->first_section)
	{
		param->first_section = FALSE;
		return;
	}

	/* create new section & append it to the section list */
	element = g_slist_last(param->sections);
	g_assert(element != NULL);

	parent = (Section *)element->data;
	g_assert(parent != NULL);

	g_debug("%s: adding section: \"%s\"::\"%s\"", __func__, section_get_name(parent), name);
	child = section_append_child(parent, name);
	param->element = PARSER_ELEMENT_SECTION;
	param->sections = g_slist_append(param->sections, child);
	param->value = NULL;
}

static void
_config_process_value_data(_ConfigDeserializationData *param, const gchar *name, ValueDatatype datatype)
{
	GSList *element;
	Section *section;

	g_assert(param != NULL);
	g_assert(param->sections != NULL);
	g_return_if_fail(param->err == NULL);

	param->datatype = datatype;

	/* get current section from stack */
	element = g_slist_last(param->sections);
	g_assert(element != NULL);

	section = (Section *)element->data;
	g_assert(section != NULL);

	/* append new value to section */
	g_debug("%s: adding value: \"%s\"::\"%s\"", __func__, section_get_name(section), name);
	if((param->value = section_append_value(section, name, datatype)))
	{
		param->element = PARSER_ELEMENT_VALUE;

		/* initialize array */
		if(VALUE_IS_ARRAY(param->value))
		{
			param->array.size = CONFIG_PARSER_ARRAY_BLOCK_SIZE;
			param->array.length = 0;
			param->array.element_size = 0;

			switch(datatype)
			{
				case VALUE_DATATYPE_INT32_ARRAY:
					param->array.element_size = sizeof(gint32);
					break;

				case VALUE_DATATYPE_FLOAT_ARRAY:
					param->array.element_size = sizeof(gfloat);
					break;

				case VALUE_DATATYPE_BOOLEAN_ARRAY:
					param->array.element_size = sizeof(gboolean);
					break;

				case VALUE_DATATYPE_STRING_ARRAY:
					param->array.element_size = sizeof(gchar *);
					break;

				default:
					g_log(NULL, G_LOG_LEVEL_WARNING, "Couldn't initialize array (param->datatype=%d)", param->datatype);
			}

			param->array.data = g_malloc(param->array.element_size * CONFIG_PARSER_ARRAY_BLOCK_SIZE);
		}
	}
	else
	{
		g_log(NULL, G_LOG_LEVEL_WARNING, "Couldn't append value (name=\"%s\", datatype=%d)", name, datatype);
	}
}

static void
_config_parse_data_start_element(GMarkupParseContext *context, const gchar *element_name,
                                 const gchar **attribute_names, const gchar **attribute_values,
				 gpointer user_data, GError **err)
{
	_ConfigDeserializationData *param = (_ConfigDeserializationData *)user_data;
	const gchar *name = NULL;
	ValueDatatype datatype = 0;

	g_return_if_fail(param->err == NULL);

	param->element = PARSER_ELEMENT_IGNORE;

	if(!g_strcmp0(element_name, "section") && _config_parse_section(attribute_names, attribute_values, &name))
	{
		_config_process_section_data(param, name);
		return;
	}

	if(!g_strcmp0(element_name, "value") && _config_parse_value(attribute_names, attribute_values, &name, &datatype))
	{
		_config_process_value_data(param, name, datatype);
		return;
	}

	if(!g_strcmp0(element_name, "item"))
	{
		param->element = PARSER_ELEMENT_ITEM;
	}
}

static void
_config_parse_data_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	_ConfigDeserializationData *param = (_ConfigDeserializationData *)user_data;

	g_return_if_fail(param->err == NULL);

	/* convert text to value */
	if(param->element == PARSER_ELEMENT_VALUE)
	{
		g_assert(param->value != NULL);

		switch(param->datatype)
		{
			case VALUE_DATATYPE_INT32:
				value_set_int32(param->value, (gint32)strtol(text, NULL, 10));
				break;

			case VALUE_DATATYPE_FLOAT:
				value_set_float(param->value, g_strtod(text, NULL));
				break;

			case VALUE_DATATYPE_BOOLEAN:
				if(!g_strcmp0(text, "true"))
				{
					value_set_bool(param->value, TRUE);
				}
				else
				{
					value_set_bool(param->value, FALSE);
				}
				break;

			case VALUE_DATATYPE_STRING:
				value_set_string(param->value, text);
				break;

			default:
				if(!VALUE_IS_ARRAY(param->value))
				{
					g_log(NULL, G_LOG_LEVEL_WARNING, "Couldn't parse text (datatype=%d)", param->datatype);
				}
		}
	}
	else
	{
		/* append text to value array */
		if(param->element == PARSER_ELEMENT_ITEM)
		{
			/* resize array */
			if(VALUE_IS_ARRAY(param->value))
			{
				if(param->array.length == param->array.size - 1)
				{
					param->array.size += CONFIG_PARSER_ARRAY_BLOCK_SIZE;
					param->array.data = g_realloc(param->array.data, param->array.size * param->array.element_size);
				}
			}

			switch(param->datatype)
			{
				case VALUE_DATATYPE_INT32_ARRAY:
					((gint32 *)param->array.data)[param->array.length] = (gint32)strtol(text, NULL, 10);
					break;

				case VALUE_DATATYPE_FLOAT_ARRAY:
					((gfloat *)param->array.data)[param->array.length] = (gint32)g_strtod(text, NULL);
					break;

				case VALUE_DATATYPE_BOOLEAN_ARRAY:
					if(!g_strcmp0(text, "true"))
					{
						((gboolean *)param->array.data)[param->array.length] = TRUE;
					}
					else
					{
						((gboolean *)param->array.data)[param->array.length] = FALSE;
					}
					break;

				case VALUE_DATATYPE_STRING_ARRAY:
					((gchar **)param->array.data)[param->array.length] = g_strdup(text);
					break;

				default:
					g_log(NULL, G_LOG_LEVEL_WARNING, "Couldn't parse text (datatype=%d)", param->datatype);
			}

			++param->array.length;
		}
	}

	/* reset element type */
	param->element = PARSER_ELEMENT_IGNORE;
}

static void
_config_parse_data_end_element(GMarkupParseContext *context, const char *element_name, gpointer user_data, GError **err)
{
	_ConfigDeserializationData *param = (_ConfigDeserializationData *)user_data;
	GSList *element;
	Section *section;

	g_return_if_fail(param->err == NULL);

	if(!g_strcmp0(element_name, "section"))
	{
		/* remove last section from the section list */
		element = g_slist_last(param->sections);
		g_assert(element != NULL);

		section = (Section *)element->data;
		param->sections = g_slist_remove(param->sections, section);
		return;
	}
	else
	{
		if(!g_strcmp0(element_name, "items"))
		{
			/* set array value & free allocated memory */
			value_set_array(param->value, param->array.data, param->array.length);

			if(VALUE_IS_STRING_ARRAY(param->value))
			{
				for(guint32 i = 0; i < param->array.length; ++i)
				{
					g_free((((gchar **)param->array.data)[i]));
				}
			}
			g_free(param->array.data);
		}
	}
}

/*
 *	implementation:
 */
static void
_config_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	Config *self = (Config *)object;

	switch(property_id)
	{
		case PROP_FILENAME:
			if(self->priv.filename)
			{
				g_free(self->priv.filename);
			}
			self->priv.filename = g_value_dup_string(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}

static void
_config_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	Config *self = (Config *)object;

	switch(property_id)
	{
		case PROP_FILENAME:
			g_value_set_string(value, self->priv.filename);
			break;

		case PROP_ROOT:
			g_value_set_object(value, self->priv.root);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	}
}
static Section *
_config_get_root(Config *self)
{
	return self->priv.root;
}

static gboolean
_config_load(Config *self, const gchar *filename, GError **err)
{
	gchar *data;
	gsize len;
	gboolean success = FALSE;

	g_assert(self->priv.root != NULL);

	/* create empty root section */
	section_unref(self->priv.root);
	self->priv.root = section_new("config");

	/* check if file does exist */
	if(g_file_test(filename, G_FILE_TEST_IS_REGULAR))
	{
		/* read file contents & parse data */
		if(g_file_get_contents(filename, &data, &len, err))
		{
			success = _config_parse_data(self->priv.root, data, len, err);
			g_free(data);
		}

		g_object_set(G_OBJECT(self), "filename", filename, NULL);
	}
	else
	{
		g_set_error(err, 0, 0, "Couldn't find file: \"%s\"", filename);
	}
	
	return success;
}

static gboolean
_config_save(Config *self, GError **err)
{
	g_return_val_if_fail(self->priv.filename != NULL, FALSE);

	return _config_save_as(self, self->priv.filename, err);
}

static gboolean
_config_save_as(Config *self, const gchar *filename, GError **err)
{
	GIOChannel *channel;
	GIOStatus status;
	gboolean success = FALSE;

	g_assert(self->priv.root != NULL);

	/* open file channel */
	if((channel = g_io_channel_new_file(filename, "w", err)))
	{
		/* serialize configuration */
		g_io_channel_write_chars(channel, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<config>\n", -1, NULL, NULL);
		success = _config_serialize(channel, self->priv.root, err);
		g_io_channel_write_chars(channel, "</config>", -1, NULL, NULL);

		/* close channel */
		status = (err && *err) ? g_io_channel_shutdown(channel, TRUE, NULL)
		                       : g_io_channel_shutdown(channel, TRUE, err);
		g_io_channel_unref(channel);
		
		if(status != G_IO_STATUS_NORMAL)
		{
			success = FALSE;
		}

		g_object_set(G_OBJECT(self), "filename", filename, NULL);
	}

	return success;
}

static const gchar *
_config_get_filename(Config *self)
{
	return self->priv.filename;
}

/*
 *	public:
 */
Config *
config_new(void)
{
	Config *instance = NULL;

	/* create instance & initialize data */
	instance = (Config *)g_object_new(CONFIG_TYPE, NULL);

	return instance;
}

void
config_unref(Config *self)
{
	g_object_unref(self);
}

Section *
config_get_root(Config *self)
{
	return CONFIG_GET_CLASS(self)->get_root(self);
}

const gchar *
config_get_filename(Config *self)
{
	return CONFIG_GET_CLASS(self)->get_filename(self);
}

gboolean
config_load(Config *self, const gchar *filename, GError **err)
{
	return CONFIG_GET_CLASS(self)->load(self, filename, err);
}

gboolean
config_save(Config *self, GError **err)
{
	return CONFIG_GET_CLASS(self)->save(self, err);
}

gboolean
config_save_as(Config *self, const gchar *filename, GError **err)
{
	return CONFIG_GET_CLASS(self)->save_as(self, filename, err);
}

/**
 * @}
 * @}
 */

