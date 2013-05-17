function exponential_fit (filename, outfile, format)

d = load (filename);

leasqrfunc = @(x, p) p(1) * exp (p(2) * x) - 220;
leasqrdfdp = @(x, f, p, dp, func) [exp(p(2)*x), p(1)*x.*exp(p(2)*x)];

t = [0:1:2000];
p = [250 0.00078579];
%p = [1 5];


data = leasqrfunc (t, p);
wt1 = (1 + 0 * t) ./ sqrt (data); 


size(data)

h=figure;
  plot (t, data);
  hold on;
  plot (d(:,1), d(:,2), 'r*');

xlabel ('Exposure time in milliseconds for DARK CURRENT MODELLING');
ylabel ('Mean pixel value in the center 100x100 image');
saveas (h, outfile, format);

F = leasqrfunc;
dFdp = leasqrdfdp; % exact derivative
%dFdp = dfdp;     % estimated derivative
dp = [0.001; 0.001];
pin = [.8; .05]; 
stol=0.001; niter=50;
minstep = [0.01; 0.01];
maxstep = [0.8; 0.8];
options = [minstep, maxstep];

%[f1, p1, kvg1, iter1, corp1, covp1, covr1, stdresid1, Z1, r21] = ...
%leasqr (t, data, pin, F, stol, niter, wt1, dp, dFdp, options);
