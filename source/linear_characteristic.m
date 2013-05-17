function linear_charactersitic (filename, outfile, format, wavelength)

d = load(filename);
size (d)
x = linspace (min(d(:,1)), max(d(:,1)), 100);
pos1=1;
pos2=18;
p = polyfit (d(pos1:pos2,1), d(pos1:pos2,2), 1);
fprintf ('min = %f, max = %f\n', min (d(:,1)), max (d(:,1)));
y = polyval (p, x);
h=figure;
plot (d(:,1), d(:,2), 'r*');
hold on;
plot (x, y);
str = '';
str = sprintf ('Exposure time in milliseconds for WAVELENGTH = %s', wavelength);
xlabel (str);
ylabel ('Mean pixel value in the center 100x100 image');
saveas (h, outfile, format);

f = fopen ('/home/aravindhan/out.txt', 'w');

fprintf ('-----------------------------------------------------------------------\n');
fprintf ('Expsoure\tDN\t\tModel DN\tpercentage deviation\n');
fprintf ('-----------------------------------------------------------------------\n');
for i=1:size (d,1)
  actual_val = d(i,2);
  model_val = polyval (p, d(i,1));
  percentage = (abs (actual_val - model_val) / model_val ) * 100;
  fprintf ('%f\t%f\t%f\t%f\n', d(i,1), actual_val, model_val, percentage);
  fprintf (f, '%f\t%f\t%f\t%f\n', d(i,1), actual_val, model_val, percentage);
end

fclose (f);

