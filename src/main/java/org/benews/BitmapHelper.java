package org.benews;

import android.graphics.Bitmap;

/**
 * Created by zad on 31/10/14.
 */
public class BitmapHelper {
	public static final float IMG_PREVIEW_LIMIT_HIGHT = 50.0f;
	public static int img_preview_limit_high=0;
	public static final float IMG_VIEW_LIMIT_HIGHT = 150.0f;
	public static int img_view_limit_high=0;
	public static float dp2dpi_factor=0;
	public static float density=0;


	static public void init(float density)
	{
		BitmapHelper.density = density;
		if(img_preview_limit_high==0) {
			// Convert the dps to pixels, based on density scale
			dp2dpi_factor = density + 0.5f;
			img_preview_limit_high = (int) (IMG_PREVIEW_LIMIT_HIGHT * dp2dpi_factor );
			img_view_limit_high = (int) (IMG_VIEW_LIMIT_HIGHT * dp2dpi_factor );
		}
	}
	// Scale and keep aspect ratio
	static public Bitmap scaleToFitWidth(Bitmap b, int width) {
		float factor = width / (float) b.getWidth();
		return Bitmap.createScaledBitmap(b, width, (int) (b.getHeight() * factor), false);
	}

	// Scale and keep aspect ratio
	static public Bitmap scaleToFitHeight(Bitmap b, int height) {
		float factor = height / (float) b.getHeight();
		return Bitmap.createScaledBitmap(b, (int) (b.getWidth() * factor), height, false);
	}

	// Scale and keep aspect ratio
	static public Bitmap scaleToFill(Bitmap b, int width, int height) {
		float factorH = height / (float) b.getWidth();
		float factorW = width / (float) b.getWidth();
		float factorToUse = (factorH > factorW) ? factorW : factorH;
		return Bitmap.createScaledBitmap(b, (int) (b.getWidth() * factorToUse), (int) (b.getHeight() * factorToUse), false);
	}

	// Scale and dont keep aspect ratio
	static public Bitmap strechToFill(Bitmap b, int width, int height) {
		float factorH = height / (float) b.getHeight();
		float factorW = width / (float) b.getWidth();
		return Bitmap.createScaledBitmap(b, (int) (b.getWidth() * factorW), (int) (b.getHeight() * factorH), false);
	}
}