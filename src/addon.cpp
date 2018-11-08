
#include "addon.h"
#include "matrix.h"
#include "text-animation.h"
#include "gif-animation.h"

Matrix *Addon::_matrix = NULL;
Animation *Addon::_currentAnimation = NULL;


struct ANIMATION_CONTEXT {
    uv_work_t request;
    Nan::Callback *callback;
    Animation *animation;
};


void Addon::runAnimation(Animation *animation, v8::Local<v8::Value> callback)
{
	ANIMATION_CONTEXT *context = new ANIMATION_CONTEXT();
	context->request.data = context;
	context->callback     = callback->IsFunction() ? new Nan::Callback(v8::Local<v8::Function>::Cast(callback)) : NULL;
	context->animation    = animation;

	uv_queue_work(uv_default_loop(), &context->request, Addon::startAnimation, Addon::animationCompleted);

}

void Addon::startAnimation(uv_work_t *request)
{
    ANIMATION_CONTEXT *context = static_cast<ANIMATION_CONTEXT*>(request->data);

	if (_currentAnimation != 0)
		_currentAnimation->stop();

	_currentAnimation = context->animation;

	_currentAnimation->run();
}


void Addon::animationCompleted(uv_work_t *request, int status)
{
	Nan::HandleScope scope;

   // This is what is called after the 'Work' is done, you can now move any data from
    // Baton to the V8/Nodejs space and invoke call back functions

    ANIMATION_CONTEXT *context = static_cast<ANIMATION_CONTEXT*>(request->data);

	if (context->callback != NULL) {
		v8::TryCatch try_catch;

	    context->callback->Call(0, 0);
	}

	if (context->animation == _currentAnimation)
		_currentAnimation = NULL;

	delete context->callback;
    delete context->animation;
    delete context;

}

NAN_METHOD(Addon::isRunning)
{
	info.GetReturnValue().Set(_currentAnimation != NULL ? true : false);
}

NAN_METHOD(Addon::update)
{

	if (_matrix == NULL) {
        return Nan::ThrowError("Matrix is not configured.");
	}

	_matrix->refresh();

	info.GetReturnValue().Set(Nan::Undefined());
};

NAN_METHOD(Addon::stop)
{
	Nan::HandleScope scope;

	if (_matrix == NULL) {
        return Nan::ThrowError("Matrix is not configured.");
	}

	v8::Local<v8::Value> callback  = Nan::Undefined();

	int argc = info.Length();

	if (argc > 0 && info[0]->IsFunction())
		callback = v8::Local<v8::Value>::Cast(info[0]);

	Animation *animation = new Animation(_matrix);

	animation->stop();

	runAnimation(animation, callback);

	info.GetReturnValue().Set(Nan::Undefined());
};

NAN_METHOD(Addon::configure)
{
	static int initialized = 0;

	Nan::HandleScope();

	if (!initialized) {
		Magick::InitializeMagick(NULL);
		initialized = 1;
	}


	if (info.Length() != 1 ) {
		return Nan::ThrowError("configure requires an argument.");
	}

	v8::Local<v8::Object> options = v8::Local<v8::Object>::Cast( info[ 0 ] );


	int width = options->Get(Nan::New<v8::String>("width").ToLocalChecked() )->Int32Value();
	int height = options->Get(Nan::New<v8::String>("height").ToLocalChecked() )->Int32Value();

	if (_matrix != NULL)
		delete _matrix;

	_matrix = new Matrix(width, height);

	info.GetReturnValue().Set(Nan::Undefined());
};

NAN_METHOD(Addon::drawImage)
{
	Nan::HandleScope();

	int argc = info.Length();

	if (_matrix == NULL) {
        return Nan::ThrowError("Matrix is not configured.");
	}

	if (argc < 1) {
		return Nan::ThrowError("drawImage requires at least one argument.");
	}

	v8::Local<v8::Object> image = info[0]->ToObject();


	int x = 0, y = 0, offsetX = 0, offsetY = 0;

	if (argc > 1)
		x = info[1]->IntegerValue();

	if (argc > 2)
		y = info[2]->IntegerValue();

	if (argc > 3)
		offsetX = info[3]->IntegerValue();

	if (argc > 4)
		offsetY = info[4]->IntegerValue();


    try {
		Magick::Image img;

		if (image->IsUndefined()) {
	        return Nan::ThrowError("drawImage needs an image");
	    }

		if (image->IsStringObject()) {

			v8::String::Utf8Value strg(image->ToString());
			std::string fileName = std::string(*strg);

			img.read(fileName.c_str());
		}

		else if (node::Buffer::HasInstance(image) ) {
		    Magick::Blob blob(node::Buffer::Data(image), node::Buffer::Length(image));
			img.read(blob);
	    }

	    else
			return Nan::ThrowError("drawImage needs an filename or image");


		// Convert transparent PNG:s
		Magick::Image tmp(Magick::Geometry(img.columns(), img.rows()), "black");
		tmp.composite(img, 0, 0, Magick::OverCompositeOp);

		_matrix->drawImage(tmp, x, y, offsetX, offsetY);



    }
    catch (exception &error) {
        string what = error.what();
        string message = string("Failed reading image: ") + what;

		return Nan::ThrowError(message.c_str());
    }
    catch (...) {
        return Nan::ThrowError("Unhandled error");
    }

	info.GetReturnValue().Set(Nan::Undefined());

};



NAN_METHOD(Addon::draw)
{
	Nan::HandleScope();


    typedef struct {
        uint8_t red;     // 0 - 255 */
        uint8_t green;   // 0 - 255 */
        uint8_t blue;    // 0 - 255 */
        uint8_t alpha;   // 0 - 255 */
    } RGBA;

    try {
        if (_matrix == NULL) {
            return Nan::ThrowError("Matrix is not configured.");
        }

    	int argc = info.Length();

        if (argc < 1) {
            return Nan::ThrowError("draw requires at least one argument.");
        }

        int width = _matrix->width();
        int height = _matrix->height();

/*
    	v8::Local<v8::Uint32Array> array = v8::Local<v8::Uint32Array>::Cast(info[0]);
        RGBA *data = (RGBA *)node::Buffer::Data();

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++, data++) {
                _matrix->setPixel(x, y, data->red, data->green, data->blue);
            }
        }
*/
        if (!node::Buffer::HasInstance(info[0])) {
            return Nan::ThrowTypeError("render(): expected argument to be a Buffer.");
        }

        v8::Local<v8::Object> buffer = info[0]->ToObject();
        int numBytes = (int)node::Buffer::Length(buffer);
        RGBA *data = (RGBA *)node::Buffer::Data(buffer);


        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++, data++) {
                _matrix->setPixel(x, y, data->red, data->green, data->blue);
            }
        }




    }
    catch (exception &error) {
        string what = error.what();
        string message = string("Failed reading image: ") + what;

		return Nan::ThrowError(message.c_str());
    }
    catch (...) {
        return Nan::ThrowError("Unhandled error");
    }

	info.GetReturnValue().Set(Nan::Undefined());

};


NAN_METHOD(Addon::drawPixel) {

	if (_matrix == NULL) {
        return Nan::ThrowError("Matrix is not configured.");
	}

    if (info.Length() < 5) {
        Nan::ThrowTypeError("Wrong number of arguments");
        return;
    }

	int x = info[0]->IntegerValue();
	int y = info[1]->IntegerValue();
	int r = info[2]->IntegerValue();
	int g = info[3]->IntegerValue();
	int b = info[4]->IntegerValue();

	_matrix->setPixel(x, y, r, g, b);

	info.GetReturnValue().Set(Nan::Undefined());

};




NAN_MODULE_INIT(initAddon)
{
	Nan::SetMethod(target, "configure",  Addon::configure);
	Nan::SetMethod(target, "isRunning",  Addon::isRunning);
	Nan::SetMethod(target, "runText",    Addon::runText);
	Nan::SetMethod(target, "runGif",     Addon::runGif);
	Nan::SetMethod(target, "runPerlin",  Addon::runPerlin);
	Nan::SetMethod(target, "runRain",    Addon::runRain);
	Nan::SetMethod(target, "runImage",   Addon::runImage);
	Nan::SetMethod(target, "runClock",   Addon::runClock);
	Nan::SetMethod(target, "stop",       Addon::stop);

	// Unsupported
	Nan::SetMethod(target, "drawImage",  Addon::drawImage);
	Nan::SetMethod(target, "drawPixel",  Addon::drawPixel);
	Nan::SetMethod(target, "update",     Addon::update);
	Nan::SetMethod(target, "draw",       Addon::draw);
}


NODE_MODULE(addon, initAddon)
